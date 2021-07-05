/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

inline double
Clamp01(double Value)
{
    double Result = (Value > 1.0 ? Value = 1.0 :
                     (Value < 0.0 ? Value = 0.0 : Value));
    return Result;
}

inline float
Clamp01(float Value)
{
    float Result = (Value > 1.0f ? Value = 1.0f :
                     (Value < 0.0f ? Value = 0.0f : Value));
    return Result;
}

inline b32
InClosedInterval(double Low, double Value, double High)
{
    b32 Result = Value > Low && Value < High;
    return Result;
}

inline b32
InOpenInterval(double Low, double Value, double High)
{
    b32 Result = Value >= Low && Value <= High;
    return Result;
}

static bbox
GetBBoxFromPair(vec2 P0, vec2 P1)
{
    bbox Result = {0};

    if (P0.X < P1.X) { Result.MinX = P0.X; Result.MaxX = P1.X; }
    else { Result.MinX = P1.X; Result.MaxX = P0.X; }
    if (P0.Y < P1.Y) { Result.MinY = P0.Y; Result.MaxY = P1.Y; }
    else { Result.MinY = P1.Y; Result.MaxY = P0.Y; }

    return Result;
}

static bbox
GetBBoxFromPair(vec3 P0, vec3 P1)
{
    bbox Result = GetBBoxFromPair(Vec2(P0), Vec2(P1));
    return Result;
}

static b32
BBoxesIntersect(bbox BBoxA, bbox BBoxB)
{
    b32 Result = !(BBoxA.MaxX < BBoxB.MinX || BBoxA.MinX > BBoxB.MaxX
                   || BBoxA.MaxY < BBoxB.MinY || BBoxA.MinY > BBoxB.MaxY);
    return Result;
}

static b32
BBoxesIntersect(OGREnvelope BBoxA, OGREnvelope BBoxB)
{
    b32 Result = !(BBoxA.MaxX < BBoxB.MinX || BBoxA.MinX > BBoxB.MaxX
                   || BBoxA.MaxY < BBoxB.MinY || BBoxA.MinY > BBoxB.MaxY);
    return Result;
}

static b32
PointInBBox(vec3 Point, bbox BBox)
{
    b32 Result = (BBox.MinX <= Point.X && Point.X <= BBox.MaxX
                  && BBox.MinY <= Point.Y && Point.Y <= BBox.MaxY);
    return Result;
}

static point_along
Intersection(vec2 A, vec2 B, vec2 C, vec2 D, vec2 AB)
{
    point_along Result = {{0}, DBL_MAX};

    vec2 CD = D - C;
    double R = AB.CrossProduct(CD);
    if (R != 0)
    {
        vec2 AC = C - A;
        double U = AC.CrossProduct(AB) / R;
        double T = AC.CrossProduct(CD) / R;

        if (0 <= U && U <= 1 && 0 <= T && T <= 1)
        {
            Result.Point = Vec3(A + AB*T);
            Result.LineLocation = T;
        }
    }

    return Result;
}

static double
LocationAlongLine(vec3 P0, vec3 P1, vec3 Target)
{
    vec2 LineDir = Vec2(P1.X-P0.X, P1.Y-P0.Y);
    vec2 TargetDir = Vec2(Target.X-P0.X, Target.Y-P0.Y);
    vec2 LineDirNorm = LineDir.Normalise(LineDir.Magnitude());
    vec2 TargetDirNorm = TargetDir.Normalise(LineDir.Magnitude());

    double Result = LineDirNorm.DotProduct(TargetDirNorm);
    return Result;
}

static vec3
PointAlongLine(vec3 P0, vec3 P1, double LineLocation)
{
    vec3 Result = ((P1 - P0) * LineLocation) + P0;
    return Result;    
}

static vec3
MovePointAlongLine(vec3 Point, vec3 OtherPoint, double PixelSize)
{
    double Distance = DistHaversine(Point, OtherPoint);
    if (Distance > PixelSize) Distance = PixelSize / Distance;
    else Distance /= 2;

    vec3 Direction = OtherPoint - Point;
    Direction *= Distance;
    vec3 Result = Point + Direction;

    return Result;
}
