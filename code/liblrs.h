#if !defined(LIBLRS_H)
/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */
#define LIBLRS_H

namespace liblrs
{

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef long long i64;
typedef int b32;

#ifdef LIBLRS_DEBUG
#include <math.h>
#endif

#include "liblrs-spatial.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))
#define Equals(ValueA, ValueB, Count) !(memcmp((ValueA), (ValueB), (Count)))
#define MoveMemoryAhead(_Ptr, _Offset, _Count) \
    for (i64 _Idx = (_Count)-1; _Idx >= 0; _Idx--) \
    { *((_Ptr) + _Idx + (_Offset)) = *((_Ptr) + _Idx); }
#define MoveMemoryBack(_Ptr, _Offset, _Count) \
    for (i64 _Idx = 0; _Idx < _Count; _Idx++) \
    { *((_Ptr) + _Idx - (_Offset)) = *((_Ptr) + _Idx); }


#pragma pack(push, 1)
struct wkb_header
{
    u8 ByteOrder;
    u32 Type;
    u32 Count;
};
#pragma pack(pop)

struct raster_info
{
    int Id;
    GDALDatasetH Raster;
    GDALRasterBandH Band;
    OGRSpatialReferenceH SRS;
    vec2 RasterSize;
    vec2 TopLeft;
    vec2 BottomRight;
    vec2 PixelSize;
};

enum boundary_field
{
    BoundaryField_Id,
    BoundaryField_EPSG,
    BoundaryField_Path,
    BoundaryField_BBox
};

struct mde_boundary
{
    wkb_header* Geometry;
    int NumVertices;
    bbox BBox;
    raster_info MDE;
};

struct point_along
{
    vec3 Point;
    double LineLocation = FLT_MAX;
};

inline b32 operator>(point_along P0, point_along P1) { return P0.LineLocation > P1.LineLocation; }
inline b32 operator>=(point_along P0, point_along P1) { return P0.LineLocation >= P1.LineLocation; }
inline b32 operator<(point_along P0, point_along P1) { return P0.LineLocation < P1.LineLocation; }
inline b32 operator<=(point_along P0, point_along P1) { return P0.LineLocation <= P1.LineLocation; }
inline b32 operator==(point_along P0, point_along P1) { return P0.LineLocation == P1.LineLocation; }
inline b32 operator!=(point_along P0, point_along P1) { return P0.LineLocation != P1.LineLocation; }

struct interval
{
    point_along P0, P1;
    raster_info MDE;

    b32 AddPoint(vec3 Point, double LineLocation)
    {
        b32 IntervalFinished = false;
        if (P0.LineLocation == FLT_MAX) P0 = { Point, LineLocation };
        else { P1 = { Point, LineLocation }; IntervalFinished = true; }
        return IntervalFinished;
    }

    void Reset(void)
    {
        P0 = { {0}, FLT_MAX };
        P1 = { {0}, FLT_MAX };
        MDE = {0};
    }
};

struct interval_list
{
    int MaxSize;
    int Count;
    interval* Base;

    void Insert(interval NewInterval)
    {
        int Low = 0;
        int High = Count-1;
        interval* Insert;

        // OBS(roberto): Insert do primeiro ponto.
        for (;;)
        {
            int Step = (Low + High) / 2;
            Insert = Base + Step;
            if (Insert->P1 < NewInterval.P0)
            {
                Low = Step + 1;
            }
            else if (Insert->P0 > NewInterval.P0)
            {
                High = Step - 1;
            }
            else
            {
                if (NewInterval.P0 > Insert->P0)
                {
                    point_along OldP1 = Insert->P1;
                    Insert->P1 = NewInterval.P0;
                    Insert++;

                    i64 RemainingIntervals = (Base + Count) - Insert;
                    MoveMemoryAhead(Insert, 1, RemainingIntervals);
                    Count++;

                    Insert->P0 = NewInterval.P0;
                    Insert->P1 = OldP1;
                    Insert->MDE = (Insert-1)->MDE;
                }
                break;
            }
        }

        // OBS(roberto): Insert do segundo ponto.
        for (;;)
        {
            i64 RemainingIntervals = (Base + Count) - Insert;

            // OBS(roberto): Intervalo intermediario, nao achou local de insert ainda.
            if (NewInterval.P1 > Insert->P1)
            {
                if (!Insert->MDE.Raster
                    || NewInterval.MDE.PixelSize.X <= Insert->MDE.PixelSize.X)
                {
                    Insert->MDE = NewInterval.MDE;
                }
                Insert++;
            }

            // OBS(roberto): Achou o local de insert do P1.
            else 
            {
                if (NewInterval.P1 == Insert->P1)
                {
                    if (!Insert->MDE.Raster
                        || NewInterval.MDE.PixelSize.X <= Insert->MDE.PixelSize.X)
                    {
                        Insert->MDE = NewInterval.MDE;
                    }
                }
                else if (NewInterval.P1 > Insert->P0)
                {
                    MoveMemoryAhead(Insert, 1, RemainingIntervals);
                    Count++;

                    Insert->P0 = Base == Insert ? Insert->P0 : (Insert-1)->P1;
                    Insert->P1 = NewInterval.P1;
                    if (!Insert->MDE.Raster
                        || NewInterval.MDE.PixelSize.X <= Insert->MDE.PixelSize.X)
                    {
                        Insert->MDE = NewInterval.MDE;
                    }

                    (Insert+1)->P0 = NewInterval.P1;
                }
                break;
            }
        }

        // OBS(roberto): Passa pela lista dando merge em celulas contiguas com o mesmo MDE.

        for (i64 Idx = 0; Idx < Count-1; )
        {
            interval* Current = Base + Idx;
            interval* Next = Base + Idx + 1;
            if (Current->MDE.Raster == Next->MDE.Raster)
            {
                Next->P0 = Current->P0;

                MoveMemoryBack(Next, 1, Count - (Idx + 1));
                Count--;
            }
            else
            {
                Idx++;
            }
        }
    }
};

typedef vec3(*fx_trans)(vec3);
typedef double(*fx_dist)(vec3, vec3);

struct calc_info
{
    char* Name;
    u32 EPSG;
    fx_trans Transform;
    fx_dist Distance;    
};

struct time_interval
{
    u64 T0, T1;
    float Diff;
};

};

#endif
