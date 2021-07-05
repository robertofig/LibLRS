/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

#define PI 3.14159265358979
#define RADIANS 0.01745329251994

#define MEAN_EARTH_RADIUS 6371000.0
#define SEMI_MAJOR_AXIS 6378137.0
#define SEMI_MINOR_AXIS 6356752.3141
#define FIRST_ECCENTRICITY 0.0066943800229
#define INV_FIRST_ECCENTRICITY 0.9933056199771

inline vec3
TransformSphereToXYZ(vec3 LL)
{
    vec2 _LL = Vec2(LL.X, -90.0-LL.Y) * RADIANS;

    double LLCosLon = cos(_LL.Lon);
    double LLCosLat = cos(_LL.Lat);
    double LLSinLon = sin(_LL.Lon);
    double LLSinLat = sin(_LL.Lat);

    vec3 XYZ = Vec3(LLSinLat * LLCosLon,
                    LLSinLat * LLSinLon,
                    LLCosLat);
    return XYZ;
}

extern "C" vec3
TransformGRS803DToXYZ(vec3 LL)
{
    double CosLon = cos(LL.Lon * RADIANS);
    double CosLat = cos(LL.Lat * RADIANS);
    double SinLon = sin(LL.Lon * RADIANS);
    double SinLat = sin(LL.Lat * RADIANS);

    double NLat = LL.Alt + (SEMI_MAJOR_AXIS / sqrt(1.0 - FIRST_ECCENTRICITY * (SinLat * SinLat)));
    double NLatCosLat = NLat * CosLat;

    vec3 XYZ = Vec3(NLatCosLat * CosLon,
                    NLatCosLat * SinLon,
                    NLat * INV_FIRST_ECCENTRICITY * SinLat);
    return XYZ;
}

extern "C" vec3
TransformGRS802DToXYZ(vec3 LL)
{
    double CosLon = cos(LL.Lon * RADIANS);
    double CosLat = cos(LL.Lat * RADIANS);
    double SinLon = sin(LL.Lon * RADIANS);
    double SinLat = sin(LL.Lat * RADIANS);

    double NLat = SEMI_MAJOR_AXIS / sqrt(1.0 - FIRST_ECCENTRICITY * (SinLat * SinLat));
    double NLatCosLat = NLat * CosLat;

    vec3 XYZ = Vec3(NLatCosLat * CosLon,
                    NLatCosLat * SinLon,
                    NLat * INV_FIRST_ECCENTRICITY * SinLat);
    return XYZ;
}

extern "C" double
DistHaversine(vec3 P1, vec3 P2)
{
    double P1Lat = P1.Lat * RADIANS;
    double P2Lat = P2.Lat * RADIANS;
    double DLat = P1Lat - P2Lat;
    double DLon = (P1.Lon - P2.Lon) * RADIANS;

    double A = pow(sin(DLat / 2.0f), 2) + pow(sin(DLon / 2.0f), 2) * cos(P1Lat) * cos(P2Lat);
    double Result = 2.0f * asin(sqrt(A)) * MEAN_EARTH_RADIUS;

    return Result;
}

extern "C" double
DistLASpherical(vec3 P1, vec3 P2)
{
    double Dot = P1.DotProduct(P2);
    double Angle = acos(Dot);

    double Result = Angle * MEAN_EARTH_RADIUS;
    return Result;
}

extern "C" double
DistLAEllipsoidal(vec3 P1, vec3 P2)
{
    double Dot = P1.DotProduct(P2);
    double P1Mag = P1.Magnitude();
    double P2Mag = P2.Magnitude();
    double Angle = acos(Dot / (P1Mag * P2Mag));

    double Result = Angle * MEAN_EARTH_RADIUS;
    return Result;
}

extern "C" double
Dist2D(vec3 P1, vec3 P2)
{
    double Result = Vec2(P1.X - P2.X, P1.Y - P2.Y).Magnitude();
    return Result;
}

extern "C" double
Dist3D(vec3 P1, vec3 P2)
{
    double Result = (P1 - P2).Magnitude();
    return Result;
}
