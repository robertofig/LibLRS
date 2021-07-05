/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

#include "gdal.h"
#include "ogr_srs_api.h"

#include "liblrs.h"

using namespace liblrs;

#include "liblrs-platform.cpp"
#include "liblrs-dist-methods.cpp"
#include "liblrs-spatial.cpp"

inline char*
JoinPaths(char* Base, size_t BaseSize, char* Append)
{
    strcpy(Base + BaseSize, Append);
    return Base;
}

static void
FreeMDEList(interval_list* List)
{
    if (List)
    {
        interval* Base = (interval*)(List + 1);
        for (int IntervalIdx = 0; IntervalIdx < List->Count; IntervalIdx++)
        {
            if ((Base + IntervalIdx)->MDE.Raster)
            GDALClose((Base + IntervalIdx)->MDE.Raster);
        }
        FreeHeapMemory(List);
    }
}

static raster_info
LoadRaster(const char* Path)
{
    raster_info Result = {0};

    Result.Raster = GDALOpenEx(Path, GDAL_OF_RASTER, NULL, NULL, NULL);
    Result.Band = GDALGetRasterBand(Result.Raster, 1);
    Result.SRS = GDALGetSpatialRef(Result.Raster);
    Result.RasterSize = Vec2(GDALGetRasterXSize(Result.Raster), GDALGetRasterYSize(Result.Raster));

    double Geotr[6] = {0};
    GDALGetGeoTransform(Result.Raster, &Geotr[0]);
    Result.PixelSize = Vec2(Geotr[1], Geotr[5]);
    Result.TopLeft = Vec2(Geotr[0], Geotr[3]);
    Result.BottomRight = Vec2(Result.TopLeft.X + (Result.RasterSize.X * Result.PixelSize.X),
                              Result.TopLeft.Y + (Result.RasterSize.Y * Result.PixelSize.Y));

    return Result;
}

extern "C" vec3
LocatePointFromMeasurement(OGRGeometryH Geom, double Measurement)
{
    vec3 Result = Vec3(NAN, NAN, NAN);

    int NumVertices = OGR_G_GetPointCount(Geom);
    double FirstM = OGR_G_GetM(Geom, 0);
    double LastM = OGR_G_GetM(Geom, NumVertices-1);

    if (FirstM <= Measurement && Measurement <= LastM)
    {
        int Low = 0;
        int High = NumVertices-1;
        int TestIdx;

        for (;;)
        {
            TestIdx = (Low + High) / 2;
            double Test = OGR_G_GetM(Geom, TestIdx);
            if (Measurement < Test)
            {
                High = TestIdx - 1;
            }
            else if (Measurement == Test)
            {
                OGR_G_GetPoint(Geom, TestIdx, &Result.X, &Result.Y, &Result.Z);
                break;
            }
            else
            {
                double Next = OGR_G_GetM(Geom, TestIdx+1);
                if (Next < Measurement)
                {
                    Low = TestIdx + 1;
                }
                else if (Next == Measurement)
                {
                    OGR_G_GetPoint(Geom, TestIdx+1, &Result.X, &Result.Y, &Result.Z);
                    break;
                }
                else
                {
                    vec3 P0, P1;
                    OGR_G_GetPoint(Geom, TestIdx, &P0.X, &P0.Y, &P0.Z);
                    OGR_G_GetPoint(Geom, TestIdx+1, &P1.X, &P1.Y, &P1.Z);

                    double LineLocation = (Next - Test) / Measurement;
                    Result = PointAlongLine(P0, P1, LineLocation);
                    break;
                }
            }
        }
    }

    return Result;
}

extern "C" double
GetMeasurementAtPoint(OGRGeometryH Geom, vec3 Point)
{
    double Result = NAN;

    int WkbSize = OGR_G_WkbSize(Geom);
    wkb_header* Linestring = (wkb_header*)GetSystemMemory(WkbSize);
    OGR_G_ExportToWkb(Geom, wkbNDR, (u8*)Linestring);
    vec3* Base = (vec3*)(Linestring + 1);

    double MinDist = DBL_MAX;
    vec3* P0 = Base;

    for (size_t VertexIdx = 1; VertexIdx < Linestring->Count; VertexIdx++)
    {
        vec3* P1 = Base + VertexIdx;
        double LineLocation = LocationAlongLine(*P0, *P1, Point);

        vec3 Target;
        if (LineLocation < 0) Target = *P0;
        else if (1 < LineLocation) Target = *P1;
        else
        {
            Target = PointAlongLine(*P0, *P1, LineLocation);
            Target.M = P0->M + ((P1->M - P0->M) * LineLocation);
        }

        double Dist = DistHaversine(Target, Point);
        if (Dist < MinDist)
        {
            MinDist = Dist;
            Result = Target.M;
        }

        P0 = P1;
    }

    FreeSystemMemory(Linestring);

    return Result;
}

extern "C" interval_list
PrepareRasters(OGRGeometryH InGeom, OGRLayerH MDEBoundaries)
{
    OGREnvelope InEnv;
    OGR_G_GetEnvelope(InGeom, &InEnv);

    int InWkbSize = OGR_G_WkbSize(InGeom);
    wkb_header* InLinestring = (wkb_header*)GetSystemMemory(InWkbSize);
    OGR_G_ExportToWkb(InGeom, wkbNDR, (u8*)InLinestring);

    vec3* InBase = (vec3*)(InLinestring + 1);
    interval_list IntervalList = {0};

    int MDECount = (int)OGR_L_GetFeatureCount(MDEBoundaries, true);
    void* Boundaries = GetHeapMemory(MDECount * sizeof(mde_boundary));
    int BoundaryCount = 0;

    // OBS(roberto): Filtra os MDEs que estao dentro da [InGeom], e adiciona eles a lista.
    OGRFeatureH MDEBoundary = NULL;
    OGR_L_ResetReading(MDEBoundaries);
    while ((MDEBoundary = OGR_L_GetNextFeature(MDEBoundaries)) != NULL)
    {
        OGRGeometryH BoundaryGeom = OGR_F_GetGeometryRef(MDEBoundary);
        OGREnvelope BoundaryEnv;
        OGR_G_GetEnvelope(BoundaryGeom, &BoundaryEnv);

        if (BBoxesIntersect(InEnv, BoundaryEnv))
        {
            mde_boundary* Boundary = (mde_boundary*)Boundaries + BoundaryCount++;
            size_t BoundarySize = OGR_G_WkbSize(BoundaryGeom);
            Boundary->Geometry = (wkb_header*)GetHeapMemory(BoundarySize);
            OGR_G_ExportToWkb(BoundaryGeom, wkbNDR, (u8*)Boundary->Geometry);
            Boundary->NumVertices = *(u32*)(Boundary->Geometry + 1);
            Boundary->BBox = BBox(BoundaryEnv.MinX, BoundaryEnv.MinY, BoundaryEnv.MaxX, BoundaryEnv.MaxY);

            const char* MDEPath = OGR_F_GetFieldAsString(MDEBoundary, BoundaryField_Path);
            Boundary->MDE = LoadRaster(MDEPath);
            Boundary->MDE.Id = OGR_F_GetFieldAsInteger(MDEBoundary, BoundaryField_Id);
        }

        OGR_F_Destroy(MDEBoundary);
    }

    // OBS(roberto): Numero pra alocar eh 2*n+1, sendo n o numero de rasters cujo BBox intersecciona
    // com o BBox da linha.

    IntervalList.MaxSize = (BoundaryCount * 2) + 1;
    int IntervalSize = IntervalList.MaxSize * sizeof(interval);
    IntervalList.Base = (interval*)GetHeapMemory(IntervalSize);

    IntervalList.Base->P0.Point = *(InBase + 0);
    IntervalList.Base->P0.LineLocation = 0;
    IntervalList.Base->P1.Point = *(InBase + InLinestring->Count-1);
    IntervalList.Base->P1.LineLocation = (float)(InLinestring->Count-1);
    IntervalList.Count = 1;

    for (size_t BoundaryIdx = 0; BoundaryIdx < BoundaryCount; BoundaryIdx++)
    {
        mde_boundary* Poly = (mde_boundary*)Boundaries + BoundaryIdx;
        vec3* L0 = InBase;
        vec3* L1 = NULL;
        interval NewInterval = {0};

        // OBS(roberto): Caso especial do primeiro ponto estar dentro do raster.
        if (PointInBBox(*L0, Poly->BBox))
        {
            NewInterval.AddPoint(*L0, 0.0f);
        }

        for (size_t LineVertexIdx = 1; LineVertexIdx < InLinestring->Count; LineVertexIdx++)
        {
            L1 = InBase + LineVertexIdx;
            bbox LBBox = GetBBoxFromPair(*L0, *L1);
            if (BBoxesIntersect(LBBox, Poly->BBox))
            {
                vec3 LDir = *L1 - *L0;
                vec2* PolyBase = (vec2*)(((u32*)(Poly->Geometry + 1))+1);

                vec2* P0 = PolyBase;
                for (int PolyVertexIdx = 1; PolyVertexIdx < Poly->NumVertices; PolyVertexIdx++)
                {
                    vec2* P1 = PolyBase + PolyVertexIdx;
                    bbox PBBox = GetBBoxFromPair(*P0, *P1);
                    if (BBoxesIntersect(LBBox, PBBox))
                    {
                        point_along Inter = Intersection(Vec2(*L0), Vec2(*L1), *P0, *P1, Vec2(LDir));
                        if (Inter.LineLocation != DBL_MAX)
                        {
                            Inter.LineLocation += (LineVertexIdx-1);

                            b32 ClosedInterval = NewInterval.AddPoint(Inter.Point, Inter.LineLocation);
                            if (ClosedInterval)
                            {
                                NewInterval.MDE = Poly->MDE;
                                IntervalList.Insert(NewInterval);
                                NewInterval.Reset();
                            }
                        }
                    }
                    P0 = P1;
                }
            }
            L0 = L1;
        }

        // OBS(roberto): Caso especial do ultimo ponto estar dentro do raster.
        if (PointInBBox(*L1, Poly->BBox))
        {
            NewInterval.AddPoint(*L1, (float)InLinestring->Count-1);
            NewInterval.MDE = Poly->MDE;
            IntervalList.Insert(NewInterval);
        }

        FreeHeapMemory(Poly->Geometry);
    }

    FreeHeapMemory(Boundaries);
    FreeSystemMemory(InLinestring);

    return IntervalList;
}

static double
GetValueFromRaster(vec3 Point, OGRGeometryH TmpGeom, OGRCoordinateTransformationH CoordTr,
                   raster_info* Raster)
{
    double Result = 0;

    OGR_G_SetPoint(TmpGeom, 0, Point.X, Point.Y, 0);
    OGR_G_Transform(TmpGeom, CoordTr);
    vec2 ProjPoint;
    OGR_G_GetPoint(TmpGeom, 0, &ProjPoint.X, &ProjPoint.Y, NULL);

    if (ProjPoint.X >= Raster->TopLeft.X
        && ProjPoint.X <= Raster->BottomRight.X
        && ProjPoint.Y <= Raster->TopLeft.Y
        && ProjPoint.Y >= Raster->BottomRight.Y)
    {
        int PixelX = (int)((ProjPoint.X - Raster->TopLeft.X) / Raster->PixelSize.X);
        int PixelY = (int)((ProjPoint.Y - Raster->TopLeft.Y) / Raster->PixelSize.Y);
        GDALRasterIO(Raster->Band, GF_Read, PixelX, PixelY, 1, 1, &Result, 1, 1, GDT_Float64, 0, 0);
    }

    return Result;
}

extern "C" OGRGeometryH
MeasureLine(OGRGeometryH InGeom, interval_list* ListPtr, calc_info Calc)
{
    int InWkbSize = OGR_G_WkbSize(InGeom);
    wkb_header* InLinestring = (wkb_header*)GetSystemMemory(InWkbSize);
    OGR_G_ExportToWkb(InGeom, wkbNDR, (u8*)InLinestring);

    vec3* Base = (vec3*)(InLinestring + 1);
    OGRGeometryH TmpPoint = OGR_G_CreateGeometry(wkbPoint);
    OGRCoordinateTransformationH CoordTr = NULL;

    OGRSpatialReferenceH InSRS = OGR_G_GetSpatialReference(InGeom);

    // OBS(roberto): Cria uma lista unica caso nenhuma tenha sido passada.

    interval_list IntervalList;
    interval TmpInterval = {0};
    if (ListPtr)
    {
        IntervalList = *ListPtr;
    }
    else
    {
        TmpInterval.P0 = { *Base, 0 };
        TmpInterval.P1 = { *(Base + InLinestring->Count-1), (double)InLinestring->Count-1 };
        IntervalList.Base = &TmpInterval;
        IntervalList.Count = 1;
    }

    OGRGeometryH OutGeometry = OGR_G_CreateGeometry(wkbLineStringM);
    int OutVerticesCount = InLinestring->Count + IntervalList.Count + 1;
    int OutWkbSize = sizeof(wkb_header) + (OutVerticesCount * sizeof(vec3));
    wkb_header* MeasuredLine = (wkb_header*)GetSystemMemory(OutWkbSize);

    vec3* MeasuredBase = (vec3*)(MeasuredLine + 1);
    vec3* MeasuredPoint = MeasuredBase;
    vec3* P0 = &(IntervalList.Base->P0.Point);
    *MeasuredPoint++ = *P0;

    double Dist = 0;
    for (int IntervalIdx = 0; IntervalIdx < IntervalList.Count; IntervalIdx++)
    {
        // OBS(roberto): O comeco e o fim da linha sempre serao o [LineLocation] do P0 e P1 do intervalo.
        // O [FirstPoint] e [LastPoint] marcam os pontos da linha original a serem contemplados. Ex: se a
        // linha inicia em 5.2 e termina em 9.6, o [FirstPoint] eh 6 e o [LastPoint] eh 9.

        interval* Interval = IntervalList.Base + IntervalIdx;
        int FirstPoint = (int)Interval->P0.LineLocation + 1;
        int LastPoint = (int)ceill(Interval->P1.LineLocation) - 1;

        if (Interval->MDE.PixelSize.X > 0)
        {
            CoordTr = OCTNewCoordinateTransformation(InSRS, Interval->MDE.SRS);

            vec3 P0Moved = MovePointAlongLine(*P0, *(Base+FirstPoint), Interval->MDE.PixelSize.X);
            P0->Z = GetValueFromRaster(P0Moved, TmpPoint, CoordTr, &(Interval->MDE));
            vec3 P0Tr = Calc.Transform(*P0);
            for (int VertexIdx = FirstPoint; VertexIdx <= LastPoint; VertexIdx++)
            {
                vec3* P1 = Base + VertexIdx;
                MeasuredPoint->X = P1->X;
                MeasuredPoint->Y = P1->Y;

                P1->Z = GetValueFromRaster(*P1, TmpPoint, CoordTr, &(Interval->MDE));
                vec3 P1Tr = Calc.Transform(*P1);

                Dist += Calc.Distance(P0Tr, P1Tr);
                (MeasuredPoint++)->M = Dist;

                P0Tr = P1Tr;
            }
            vec3* PFinal = &(Interval->P1.Point);
            MeasuredPoint->X = PFinal->X;
            MeasuredPoint->Y = PFinal->Y;

            vec3 PFinalMoved = MovePointAlongLine(*PFinal, *(Base+LastPoint), Interval->MDE.PixelSize.X);
            PFinal->Z = GetValueFromRaster(PFinalMoved, TmpPoint, CoordTr, &(Interval->MDE));
            vec3 PFinalTr = Calc.Transform(*PFinal);

            Dist += Calc.Distance(P0Tr, PFinalTr);
            (MeasuredPoint++)->M = Dist;

            P0 = PFinal;
        }
        else
        {
            vec3 P0Tr = TransformGRS802DToXYZ(*P0);
            for (int VertexIdx = FirstPoint; VertexIdx <= LastPoint; VertexIdx++)
            {
                vec3* P1 = Base + VertexIdx;
                MeasuredPoint->X = P1->X;
                MeasuredPoint->Y = P1->Y;

                vec3 P1Tr = TransformGRS802DToXYZ(*P1);
                Dist += Dist3D(P0Tr, P1Tr);
                (MeasuredPoint++)->M = Dist;

                P0Tr = P1Tr;
            }
            vec3* PFinal = &(Interval->P1.Point);
            MeasuredPoint->X = PFinal->X;
            MeasuredPoint->Y = PFinal->Y;

            vec3 PFinalTr = TransformGRS802DToXYZ(*PFinal);
            Dist += Dist3D(P0Tr, PFinalTr);
            (MeasuredPoint++)->M = Dist;

            P0 = PFinal ;
        }
    }

    MeasuredLine->ByteOrder = InLinestring->ByteOrder;
    MeasuredLine->Type = 2002;
    MeasuredLine->Count = (u32)(MeasuredPoint - MeasuredBase);
    OGR_G_ImportFromWkb(OutGeometry, MeasuredLine, OutWkbSize);

    FreeSystemMemory(MeasuredLine);
    FreeSystemMemory(InLinestring);

    return OutGeometry;
}
