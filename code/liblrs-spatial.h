#if !defined(LIBLRS_MATH_H)
/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */
#define LIBLRS_MATH_H

struct vec2
{
    union { double X, Lon; };
    union { double Y, Lat; };

    double DotProduct(vec2 B)
    {
        double Result = X*B.X + Y*B.Y;
        return Result;        
    }

    double Magnitude(void)
    {
        double Result = sqrt(X*X + Y*Y);
        return Result;
    }

    vec2 Normalise(double Mag)
    {
        vec2 Result = { X/Mag, Y/Mag };
        return Result;
    }

    double CrossProduct(vec2 B)
    {
        double Result = X*B.Y - Y*B.X;
        return Result;
    }
};

inline vec2
operator+(vec2 A, vec2 B)
{
    vec2 Result = { A.X + B.X, A.Y + B.Y };
    return Result;
}

inline vec2&
operator+=(vec2& A, vec2 B)
{
    A = A + B;
    return A;
}

inline vec2
operator-(vec2 A, vec2 B)
{
    vec2 Result = { A.X - B.X, A.Y - B.Y };
    return Result;
}

inline vec2&
operator-=(vec2& A, vec2 B)
{
    A = A - B;
    return A;
}

inline vec2
operator*(vec2 A, double Scalar)
{
    vec2 Result = { A.X * Scalar, A.Y * Scalar };
    return Result;
}

inline vec2&
operator*=(vec2& A, double Scalar)
{
    A = A * Scalar;
    return A;
}

struct vec3
{
    union { double X, Lon; };
    union { double Y, Lat; };
    union { double Z, Alt, M; };

    double DotProduct(vec3 B)
    {
        double Result = X*B.X + Y*B.Y + Z*B.Z;
        return Result;
    }

    double Magnitude(void)
    {
        double Result = sqrt(X*X + Y*Y + Z*Z);
        return Result;
    }

    vec3 Normalise(double Mag)
    {
        vec3 Result = { X/Mag, Y/Mag, Z/Mag };
        return Result;
    }
    
    double CrossProduct(vec3 B)
    {
        double Result = (Y*B.Z - Z*B.Y) - (X*B.Z - Z*B.X) + (X*B.Y - Y*B.X);
        return Result;
    }
};

inline vec3
operator+(vec3 A, vec3 B)
{
    vec3 Result = { A.X + B.X, A.Y + B.Y, A.Z + B.Z };
    return Result;
}

inline vec3&
operator+=(vec3& A, vec3 B)
{
    A = A + B;
    return A;
}

inline vec3
operator-(vec3 A, vec3 B)
{
    vec3 Result = { A.X - B.X, A.Y - B.Y, A.Z - B.Z };
    return Result;
}

inline vec3&
operator-=(vec3& A, vec3 B)
{
    A = A - B;
    return A;
}

inline vec3
operator*(vec3 A, double Scalar)
{
    vec3 Result = { A.X * Scalar, A.Y * Scalar, A.Z * Scalar };
    return Result;
}

inline vec3&
operator*=(vec3& A, double Scalar)
{
    A = A * Scalar;
    return A;
}

inline vec2
Vec2(double X, double Y)
{
    vec2 Result = { X, Y };
    return Result;
}

inline vec2
Vec2(vec3 A)
{
    vec2 Result = { A.X, A.Y };
    return Result;
}

inline vec3
Vec3(double X, double Y, double Z)
{
    vec3 Result = { X, Y, Z };
    return Result;
}

inline vec3
Vec3(vec2 A)
{
    vec3 Result = { A.X, A.Y, 0 };
    return Result;
}

struct bbox
{
    double MinX, MinY, MaxX, MaxY;
};

inline bbox
BBox(double MinX, double MinY, double MaxX, double MaxY)
{
    bbox Result = { MinX, MinY, MaxX, MaxY };
    return Result;
}

#endif
