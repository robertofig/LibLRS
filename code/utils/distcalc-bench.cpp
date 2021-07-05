/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

#include "..\\liblrs.cpp"

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

static int GlobalLoop = 100;
static const char* HelpMessage = "\
DistCalc Bench\n\
Flags:\n\
  -h, --help\t\tUtility help.\n\
  -s, --shapefile\tPath to LinestringZ shapefile (latlon). [Mandatory]\n\
  -e, --epsg\t\tEPSG code for transformation. [Mandatory]\n\
  -l, --loop\t\tNumber of times the function will loop (for timing purposes). [Optional, default 100]\n\
Example usage:\n\
  'distcalc-bench.exe -s path\\to\\shapefile.shp -e 1234 --loop 250'\n";

enum args_state
{
    ArgsState_Flag,
    ArgsState_Shapefile,
    ArgsState_EPSG,
    ArgsState_Loop
};

static b32
IsLinestring(OGRwkbGeometryType Type)
{
    b32 Result = (Type == wkbLineString25D);
    return Result;
}

static void
TimeDistance(OGRGeometryH Geom, wkb_header* Linestring, calc_info Info)
{
    time_interval Clock;

    double PreprocessLow = FLT_MAX;
    double PreprocessHigh = 0.0f;
    double PreprocessTotal = 0.0f;

    double ProcessLow = FLT_MAX;
    double ProcessHigh = 0.0f;
    double ProcessTotal = 0.0f;

    double TotalDist = 0.0f;

    for (int Count = 0; Count < GlobalLoop; Count++)
    {
        StartTiming(&Clock);
        if (Info.EPSG != 0)
        {
            OGRSpatialReferenceH NewSRS = OSRNewSpatialReference(NULL);
            OSRImportFromEPSG(NewSRS, Info.EPSG);
            OGR_G_TransformTo(Geom, NewSRS);
        }
        OGR_G_ExportToWkb(Geom, wkbNDR, (u8*)Linestring);
        TotalDist = 0.0f;
        vec3* Base = (vec3*)(Linestring + 1);
        u32 NumPoints = Linestring->Count;
        if (Info.Transform)
        {
            for (size_t PointIdx = 0; PointIdx < NumPoints; PointIdx++)
            {
                vec3* Point = Base + PointIdx;
                *Point = Info.Transform(*Point);
            }
        }
        EndTiming(&Clock);
        float Preprocess = Clock.Diff;

        StartTiming(&Clock);
        vec3* P1 = Base;
        for (size_t PointIdx = 1; PointIdx < NumPoints; PointIdx++)
        {
            vec3* P2 = P1 + 1;
            TotalDist += Info.Distance(*P1, *P2);
            P1 = P2;
        }
        EndTiming(&Clock);
        float Process = Clock.Diff;

        if (Preprocess < PreprocessLow) PreprocessLow = Preprocess;
        if (Preprocess > PreprocessHigh) PreprocessHigh = Preprocess;
        PreprocessTotal += Preprocess;
        if (Process < ProcessLow) ProcessLow = Process;
        if (Process > ProcessHigh) ProcessHigh = Process;
        ProcessTotal += Process;
    }

    double PreprocessMean = PreprocessTotal / GlobalLoop;
    double ProcessMean = ProcessTotal / GlobalLoop;
    printf("%30s:\t%.4f\t", Info.Name, TotalDist);
    printf("{%7.1f, %7.1f, %7.1f}\t", PreprocessLow, PreprocessMean, PreprocessHigh);
    printf("{%7.1f, %7.1f, %7.1f}\t", ProcessLow, ProcessMean, ProcessHigh);
    printf("{%7.1f, %7.1f, %7.1f}\n", PreprocessLow + ProcessLow, PreprocessMean + ProcessMean,
           PreprocessHigh + ProcessHigh);
}

int
main(int Argc, char* Argv[])
{
    char* ShapefilePath = NULL;
    u32 EPSG = 0;

    if (Argc > 7)
    {
        printf("Too many arguments, exiting. Use flag -h for instructions.\n");
        goto exit;
    }

    args_state State = ArgsState_Flag;
    for (size_t Idx = 1; Idx < Argc; Idx++)
    {
        char* Arg = Argv[Idx];

        switch (State)
        {
            case ArgsState_Flag:
            {
                if (Equals(Arg, "-h", 2) || Equals(Arg, "--help", 6))
                {
                    printf("%s\n", HelpMessage);
                    goto exit;
                }
                else if (Equals(Arg, "-s", 2) || Equals(Arg, "--shapefile", 11)) State = ArgsState_Shapefile;
                else if (Equals(Arg, "-e", 2) || Equals(Arg, "--epsg", 6)) State = ArgsState_EPSG;
                else if (Equals(Arg, "-l", 2) || Equals(Arg, "--loop", 6)) State = ArgsState_Loop;
                else
                {
                    printf("Incorrect flag '%s', exiting. Use flag -h for instructions.\n", Arg);
                    goto exit;
                }
            } break;

            case ArgsState_Shapefile:
            {
                ShapefilePath = Arg;
                State = ArgsState_Flag;
            } break;

            case ArgsState_EPSG:
            {
                EPSG = atoi(Arg);
                State = ArgsState_Flag;
            } break;

            case ArgsState_Loop:
            {
                GlobalLoop = atoi(Arg);
                State = ArgsState_Flag;
            } break;
        }
    }

    if (!ShapefilePath) { printf("Path of shapefile not specified. Exiting...\n"); goto exit; }
    if (EPSG == 0) { printf("EPSG code not specified. Exiting...\n"); goto exit; }

    calc_info Methods[7] = {{ "Haversine", 0, NULL, &DistHaversine },
                            { "Spherical Arc", 0, &TransformSphereToXYZ, &DistLASpherical },
                            { "Ellipsoidal Arc", 0, &TransformGRS803DToXYZ, &DistLAEllipsoidal },
                            { "Ellipsoidal Cart 2D", 0, &TransformGRS802DToXYZ, &Dist3D },
                            { "Ellipsoidal Cart 3D", 0, &TransformGRS803DToXYZ, &Dist3D },
                            { "Projected 2D", EPSG, NULL, &Dist2D },
                            { "Projected 3D", EPSG, NULL, &Dist3D }};

    GDALAllRegister();

    GDALDatasetH Dataset = GDALOpenEx(ShapefilePath, GDAL_OF_VECTOR, NULL, NULL, NULL);
    if (Dataset)
    {
        OGRLayerH Layer = GDALDatasetGetLayer(Dataset, 0);
        if (Layer)
        {
            if (IsLinestring(OGR_L_GetGeomType(Layer)))
            {
                i64 NumFeatures = OGR_L_GetFeatureCount(Layer, TRUE);
                for (i64 FeatureIdx = 0; FeatureIdx < NumFeatures; FeatureIdx++)
                {
                    OGRFeatureH Feature = OGR_L_GetNextFeature(Layer);
                    OGRGeometryH Geom = OGR_F_GetGeometryRef(Feature);

                    int WkbGeomSize = OGR_G_WkbSize(Geom);
                    wkb_header* WkbBuffer = (wkb_header*)GetSystemMemory(WkbGeomSize);
                    if (WkbBuffer)
                    {
                        for (int MethodIdx = ArrayCount(Methods)-1; MethodIdx >= 0; MethodIdx--)
                        {
                            OGRGeometryH _Geom = OGR_G_Clone(Geom);
                            TimeDistance(_Geom, WkbBuffer, Methods[MethodIdx]);
                        }
                        FreeSystemMemory(WkbBuffer);
                    }
                    OGR_F_Destroy(Feature);
                }
            }
            else
            {
                printf("Geometry is not of type LinestringZ, exiting.\n");
            }
        }
    }

exit:
    return 0;
}
