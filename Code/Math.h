#pragma once

#include "Intrinsics.h"

const float Pi = 3.14159265358979323846f;

const int32_t Int32Min = (-2147483647 - 1);
const int32_t Int32Max = 2147483647;
const uint32_t UInt32Max = 0xFFFFFFFF;
const float FloatMax = 3.402823466e+38F;
const float FloatEpsilon = 1.19209e-07F;

inline float Max(float A, float B) { return (A > B) ? A : B; }
inline uint32_t Max(uint32_t A, uint32_t B) { return (A > B) ? A : B; }
inline float Min(float A, float B) { return (A < B) ? A : B; }
inline uint32_t Min(uint32_t A, uint32_t B) { return (A < B) ? A : B; }

union vec2
{
    struct
    {
        float x, y;
    };

    struct
    {
        float u, v;
    };

    float E[2];
};

union vec3
{
    struct
    {
        float x, y, z;
    };

    struct
    {
        float u, v, w;
    };

    struct
    {
        float r, g, b;
    };

    struct 
    {
        vec2 xy;
        float Ignored_0;
    };
    struct 
    {
        float Ignored_1;
        vec2 yz;
    };
    struct 
    {
        vec2 uv;
        float Ignored_2;
    };
    struct 
    {
        float Ignored_3;
        vec2 vw;
    };

    float E[3];
};

union vec4
{
    struct
    {
        float x, y, z, w;
    };

    struct
    {
        float r, g, b, a;
    };

    struct 
    {
        vec3 xyz;
        float Ignored_0;
    };
    struct
    {
        vec2 xy;
        float Ignored_1;
        float Ignored_2;
    };
    struct
    {
        float Ignored_3;
        vec2 yz;
        float Ignored_4;
    };
    struct
    {
        float Ignored_5;
        float Ignored_6;
        vec2 zw;
    };

    struct
    {
        vec3 rgb;
        float Ignored_7;
    };

    float E[4];
};

union quat
{
    struct
    {
        float x, y, z, w;
    };

    struct
    {
        vec3 v;
        float w;
    };
};

union mat3
{
    float E[9];

    struct 
    {
        float a00, a10, a20;
        float a01, a11, a21;
        float a02, a12, a22;
    };
};

union mat4
{
    float E[16];

    struct 
    {
        float a00, a10, a20, a30;
        float a01, a11, a21, a31;
        float a02, a12, a22, a32;
        float a03, a13, a23, a33;
    };
};

struct Rect
{
    vec3 Min;
    vec3 Max;
};

//
// NOTE(georgii): scalar
//

inline float Radians(float Angle)
{
    float Result = (Angle / 180.0f) * Pi;

    return Result;
}

inline float Degrees(float Rad)
{
    float Result = (Rad / Pi) * 180.0f;

    return Result;
}

inline float Lerp(float A, float B, float t)
{
    float Result = A + (B - A)*t;

    return Result;
}

inline float Clamp(float Value, float Min, float Max)
{
    Assert(Min <= Max);

    if (Value < Min) 
    {
        Value = Min;
    }
    else if (Value > Max)
    {
        Value = Max;
    } 

    return(Value);
}

inline int32_t Clamp(int32_t Value, int32_t Min, int32_t Max)
{
    Assert(Min <= Max);

    if (Value < Min) 
    {
        Value = Min;
    }
    else if (Value > Max)
    {
        Value = Max;
    } 

    return(Value);
}

inline bool IsEqual(vec2 A, vec2 B)
{
    bool Result = (Absolute(A.x - B.x) <= FloatEpsilon) &&
                  (Absolute(A.y - B.y) <= FloatEpsilon);

    return Result;
}

// 
// NOTE(georgii): vec2
// 

inline vec2 Vec2(float X, float Y) 
{ 
    vec2 Result;
    
    Result.x = X; 
    Result.y = Y; 

    return Result;
}

inline vec2 Vec2(float Value) 
{ 
    vec2 Result;
    
    Result.x = Value; 
    Result.y = Value; 

    return Result;
}

inline vec2 Vec2i(int32_t X, int32_t Y)
{
    vec2 Result = Vec2((float)X, (float)Y);

    return Result;
}

inline vec2 Vec2i(uint32_t X, uint32_t Y)
{
    vec2 Result = Vec2((float)X, (float)Y);

    return Result;
}

inline vec2 operator+ (vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return Result;
}

inline vec2 operator- (vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return Result;
}

inline vec2 Hadamard(vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;

    return Result;
}

inline vec2 operator/ (vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x / B.x;
    Result.y = A.y / B.y;

    return Result;
}

inline vec2 operator* (vec2 A, float B)
{
    vec2 Result;

    Result.x = A.x * B;
    Result.y = A.y * B;

    return Result;
}

inline vec2 operator* (float B, vec2 A)
{
    vec2 Result = A * B;

    return Result;
}

inline vec2& operator+= (vec2& A, vec2 B)
{
    A = A + B;
    
    return A;
}

inline vec2& operator-= (vec2& A, vec2 B)
{
    A = A - B;
    
    return A;
}

inline vec2& operator*= (vec2& A, float B)
{
    A = A * B;
    
    return A;
}

inline vec2 operator- (vec2 A)
{
    vec2 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    
    return Result;
}

inline float Dot(vec2 A, vec2 B)
{
    float Result = A.x*B.x + A.y*B.y;

    return Result;
}

inline float LengthSq(vec2 A)
{
    float Result = Dot(A, A);

    return Result;
}

inline float Length(vec2 A)
{
    float Result = SquareRoot(LengthSq(A));

    return Result;
}

inline vec2 Normalize(vec2 A)
{
    vec2 Result = A * (1.0f / Length(A));

    return Result;
}

inline vec2 Perp(vec2 A)
{
    vec2 Result = Vec2(-A.y, A.x);
    
    return Result;
}

inline float Cross2D(vec2 A, vec2 B)
{
    float Result = Dot(Perp(A), B);

    return Result;
}

inline vec2 Lerp(vec2 A, vec2 B, float t)
{
    vec2 Result = A + (B - A)*t;

    return Result;
}

inline vec2 Lerp(vec2 A, vec2 B, vec2 t)
{
    vec2 Result = A + Hadamard(B - A, t);

    return Result;
}

// 
// NOTE(georgii): vec3
// 

inline vec3 Vec3(float X, float Y, float Z) 
{ 
    vec3 Result;
    
    Result.x = X; 
    Result.y = Y; 
    Result.z = Z; 

    return Result;
}

inline vec3 Vec3(float Value) 
{ 
    vec3 Result;
    
    Result.x = Value; 
    Result.y = Value; 
    Result.z = Value; 

    return Result;
}

inline vec3 Vec3(vec2 XY, float Z) 
{ 
    vec3 Result;
    
    Result.xy = XY; 
    Result.z = Z; 

    return Result;
}

inline vec3 Vec3i(int32_t X, int32_t Y, int32_t Z)
{
    vec3 Result = Vec3((float)X, (float)Y, (float)Z);

    return Result;
}

inline vec3 Vec3i(uint32_t X, uint32_t Y, uint32_t Z)
{
    vec3 Result = Vec3((float)X, (float)Y, (float)Z);

    return Result;
}

inline vec3 operator+ (vec3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;

    return Result;
}

inline vec3 operator- (vec3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;

    return Result;
}

inline vec3 Hadamard(vec3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;

    return Result;
}

inline vec3 operator* (vec3 A, float B)
{
    vec3 Result;

    Result.x = A.x * B;
    Result.y = A.y * B;
    Result.z = A.z * B;

    return Result;
}

inline vec3 operator* (float B, vec3 A)
{
    vec3 Result = A * B;

    return Result;
}

inline vec3& operator+= (vec3& A, vec3 B)
{
    A = A + B;
    
    return A;
}

inline vec3& operator-= (vec3& A, vec3 B)
{
    A = A - B;
    
    return A;
}

inline vec3& operator*= (vec3& A, float B)
{
    A = A * B;
    
    return A;
}

inline vec3 operator- (vec3 A)
{
    vec3 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    
    return Result;
}

inline float Dot(vec3 A, vec3 B)
{
    float Result = A.x*B.x + A.y*B.y + A.z*B.z;

    return Result;
}

inline float LengthSq(vec3 A)
{
    float Result = Dot(A, A);

    return Result;
}

inline float Length(vec3 A)
{
    float Result = SquareRoot(LengthSq(A));

    return Result;
}

inline vec3 Normalize(vec3 A)
{
    vec3 Result = A * (1.0f / Length(A));

    return Result;
}

inline vec3 NormalizeSafe0(vec3 A)
{
    vec3 Result = Vec3(0.0f);
    if (Length(A) > FloatEpsilon)
    {
        Result = A * (1.0f / Length(A));
    }

    return Result;
}

inline vec3 Cross(vec3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.y*B.z - B.y*A.z;
    Result.y = A.z*B.x - B.z*A.x;
    Result.z = A.x*B.y - B.x*A.y;

    return Result;   
}

inline vec3 Lerp(vec3 A, vec3 B, float t)
{
    vec3 Result = A + (B - A)*t;

    return Result;
}

inline vec3 Lerp(vec3 A, vec3 B, vec3 t)
{
    vec3 Result = A + Hadamard(B - A, t);

    return Result;
}

inline vec3 Clamp(vec3 Value, vec3 Min, vec3 Max)
{
    vec3 Result;

    Result.x = Clamp(Value.x, Min.x, Max.x);
    Result.y = Clamp(Value.y, Min.y, Max.y);
    Result.z = Clamp(Value.z, Min.z, Max.z);

    return Result;
}

inline vec3 Clamp(vec3 Value, float Min, float Max)
{
    vec3 Result;

    Result.x = Clamp(Value.x, Min, Max);
    Result.y = Clamp(Value.y, Min, Max);
    Result.z = Clamp(Value.z, Min, Max);

    return Result;
}

inline bool IsEqual(vec3 A, vec3 B)
{
    bool Result = (Absolute(A.x - B.x) <= FloatEpsilon) &&
                  (Absolute(A.y - B.y) <= FloatEpsilon) &&
                  (Absolute(A.z - B.z) <= FloatEpsilon);

    return Result;
}

// 
// NOTE(georgii): vec4
// 

inline vec4 Vec4(float X, float Y, float Z, float W) 
{ 
    vec4 Result;
    
    Result.x = X; 
    Result.y = Y; 
    Result.z = Z; 
    Result.w = W; 

    return Result;
}

inline vec4 Vec4(float Value) 
{ 
    vec4 Result;
    
    Result.x = Value; 
    Result.y = Value; 
    Result.z = Value; 
    Result.w = Value; 

    return Result;
}

inline vec4 Vec4(vec2 XY, float Z, float W) 
{ 
    vec4 Result;
    
    Result.xy = XY; 
    Result.z = Z;
    Result.w = W; 

    return Result;
}

inline vec4 Vec4(vec2 XY, vec2 ZW) 
{ 
    vec4 Result;
    
    Result.xy = XY; 
    Result.zw = ZW;

    return Result;
}

inline vec4 Vec4(vec3 XYZ, float W) 
{ 
    vec4 Result;
    
    Result.xyz = XYZ; 
    Result.w = W; 

    return Result;
}

inline vec4 Vec4i(int32_t X, int32_t Y, int32_t Z, int32_t W)
{
    vec4 Result = Vec4((float)X, (float)Y, (float)Z, (float)W);

    return Result;
}

inline vec4 Vec4i(uint32_t X, uint32_t Y, uint32_t Z, uint32_t W)
{
    vec4 Result = Vec4((float)X, (float)Y, (float)Z, (float)W);

    return Result;
}

inline vec4 operator+ (vec4 A, vec4 B)
{
    vec4 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    Result.w = A.w + B.w;

    return Result;
}

inline vec4 operator- (vec4 A, vec4 B)
{
    vec4 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    Result.w = A.w - B.w;

    return Result;
}

inline vec4 Hadamard(vec4 A, vec4 B)
{
    vec4 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;
    Result.w = A.w * B.w;

    return Result;
}

inline vec4 operator* (vec4 A, float B)
{
    vec4 Result;

    Result.x = A.x * B;
    Result.y = A.y * B;
    Result.z = A.z * B;
    Result.w = A.w * B;

    return Result;
}

inline vec4 operator* (float B, vec4 A)
{
    vec4 Result = A * B;

    return Result;
}

inline vec4& operator+= (vec4& A, vec4 B)
{
    A = A + B;
    
    return A;
}

inline vec4& operator-= (vec4& A, vec4 B)
{
    A = A - B;
    
    return A;
}

inline vec4& operator*= (vec4& A, float B)
{
    A = A * B;
    
    return A;
}

inline vec4 operator- (vec4 A)
{
    vec4 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    Result.w = -A.w;
    
    return Result;
}

inline float Dot(vec4 A, vec4 B)
{
    float Result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;

    return Result;
}

inline float LengthSq(vec4 A)
{
    float Result = Dot(A, A);

    return Result;
}

inline float Length(vec4 A)
{
    float Result = SquareRoot(LengthSq(A));

    return Result;
}

inline vec4 Normalize(vec4 A)
{
    vec4 Result = A * (1.0f / Length(A));

    return Result;
}

inline vec4 Lerp(vec4 A, vec4 B, float t)
{
    vec4 Result = A + (B - A)*t;

    return Result;
}

inline vec4 Lerp(vec4 A, vec4 B, vec4 t)
{
    vec4 Result = A + Hadamard(B - A, t);

    return Result;
}

//
// NOTE(georgii): quat
//

inline quat Quat(float X, float Y, float Z, float W)
{
    quat Result;

    Result.x = X;
    Result.y = Y;
    Result.z = Z;
    Result.w = W;
    
    return Result;
}

inline quat Quat(vec3 Axis, float Angle)
{
    quat Result;

    vec3 V = Sin(Radians(0.5f * Angle)) * Axis;
    float W = Cos(Radians(0.5f * Angle));

    Result.x = V.x;
    Result.y = V.y;
    Result.z = V.z;
    Result.w = W;
    
    return Result;
}

inline float GetAngle(quat Q)
{
	float Result = 2.0f * ArcCos(Q.w);

	return Result;
}

inline quat operator* (quat A, quat B)
{
    quat Result;

    Result.v = A.w*B.v + B.w*A.v + Cross(A.v, B.v);
    Result.w = Clamp(A.w*B.w - Dot(A.v, B.v), -1.0f, 1.0f);

    return Result;
}

inline quat EulerToQuat(vec3 Euler)
{
    quat Result = Quat(Vec3(1.0f, 0.0f, 0.0f), Euler.x) *  Quat(Vec3(0.0f, 1.0f, 0.0f), Euler.y) * Quat(Vec3(0.0f, 0.0f, 1.0f), Euler.z);

    return Result;
}

vec3 RotateByQuaternion(vec3 V, quat Q)
{
    vec3 Result = V + 2.0 * Cross(Q.v, Cross(Q.v, V) + Q.w * V);

	return Result;
}

vec4 QuaternionToAxisAngle(quat Q)
{
	float AngleRadians = GetAngle(Q);
	vec3 Axis = Vec3(SafeDivide0(Q.x, Sin(0.5f * AngleRadians)), SafeDivide0(Q.y, Sin(0.5f * AngleRadians)), SafeDivide0(Q.z, Sin(0.5f * AngleRadians)));

	vec4 Result = Vec4(Axis, Degrees(AngleRadians));

	return Result;
}

// 
// NOTE(georgii): mat3
// 

inline mat3 Identity3x3(float Diagonal = 1.0f)
{
    mat3 Result = {};

    Result.a00 = Diagonal;
    Result.a11 = Diagonal;
    Result.a22 = Diagonal;

    return Result;
}

inline mat3 Scaling3x3(float Scale)
{
    mat3 Result = {};

    Result.a00 = Scale;
    Result.a11 = Scale;
    Result.a22 = Scale;

    return Result;
}

inline mat3 Scaling3x3(vec3 Scale)
{
    mat3 Result = {};

    Result.a00 = Scale.x;
    Result.a11 = Scale.y;
    Result.a22 = Scale.z;

    return Result;
}

mat3 Rotation3x3(float Angle, vec3 Axis)
{
    mat3 Result;

    float Rad = Radians(Angle);
    float Sine = Sin(Rad);
    float Cosine = Cos(Rad);

    Assert(LengthSq(Axis) > 0.0f);
    Axis = Normalize(Axis);

    Result.a00 = Axis.x*Axis.x*(1.0f - Cosine) + Cosine;
    Result.a10 = Axis.x*Axis.y*(1.0f - Cosine) + Axis.z*Sine;
    Result.a20 = Axis.x*Axis.z*(1.0f - Cosine) - Axis.y*Sine;

    Result.a01 = Axis.x*Axis.y*(1.0f - Cosine) - Axis.z*Sine;
    Result.a11 = Axis.y*Axis.y*(1.0f - Cosine) + Cosine;
    Result.a21 = Axis.y*Axis.z*(1.0f - Cosine) + Axis.x*Sine;

    Result.a02 = Axis.x*Axis.z*(1.0f - Cosine) + Axis.y*Sine;
    Result.a12 = Axis.y*Axis.z*(1.0f - Cosine) - Axis.x*Sine;
    Result.a22 = Axis.z*Axis.z*(1.0f - Cosine) + Cosine;

    return Result;
}

mat3 Transpose3x3(const mat3 &M)
{
    mat3 Result;

    Result.a00 = M.a00;
	Result.a10 = M.a01;
	Result.a20 = M.a02;

	Result.a01 = M.a10;
	Result.a11 = M.a11;
	Result.a21 = M.a12;

	Result.a02 = M.a20;
	Result.a12 = M.a21;
	Result.a22 = M.a22;

    return Result;
}

mat3 ToMat3(const mat4 &M)
{
    mat3 Result;

    Result.a00 = M.a00; Result.a10 = M.a10; Result.a20 = M.a20;
    Result.a01 = M.a01; Result.a11 = M.a11; Result.a21 = M.a21;
    Result.a02 = M.a02; Result.a12 = M.a12; Result.a22 = M.a22;

    return Result;
}

mat3 operator*(const mat3& A, const mat3& B)
{
    mat3 Result;

    for(uint32_t Row = 0;
        Row < 3;
        Row++)
    {
        for(uint32_t Column = 0;
            Column < 3;
            Column++)
        {
            float Sum = 0.0f;
            for(uint32_t E = 0;
                E < 3;
                E++)
            {
                Sum += A.E[Row + E*3] * B.E[Column*3 + E];
            }
            Result.E[Row + Column*3] = Sum;
        }
    }

    return Result;
}

vec3 operator*(mat3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.a00*B.x + A.a01*B.y + A.a02*B.z;
    Result.y = A.a10*B.x + A.a11*B.y + A.a12*B.z;
    Result.z = A.a20*B.x + A.a21*B.y + A.a22*B.z;

    return Result;
}

// 
// NOTE(georgii): mat4
// 

inline mat4 Identity(float Diagonal = 1.0f)
{
    mat4 Result = {};

    Result.a00 = Diagonal;
    Result.a11 = Diagonal;
    Result.a22 = Diagonal;
    Result.a33 = Diagonal;

    return Result;
}

inline mat4 Translation(vec3 Translate)
{
    mat4 Result = Identity(1.0f);

    Result.a03 = Translate.x;
    Result.a13 = Translate.y;
    Result.a23 = Translate.z;

    return Result;
}

inline mat4 Scaling(float Scale)
{
    mat4 Result = {};

    Result.a00 = Scale;
    Result.a11 = Scale;
    Result.a22 = Scale;
    Result.a33 = 1.0f;

    return Result;
}

inline mat4 Scaling(vec3 Scale)
{
    mat4 Result = {};

    Result.a00 = Scale.x;
    Result.a11 = Scale.y;
    Result.a22 = Scale.z;
    Result.a33 = 1.0f;

    return Result;
}

mat4 Rotation(float Angle, vec3 Axis)
{
    mat4 Result;

    if ((Angle != 0.0f) && (LengthSq(Axis) > 0.0f))
    {
        float Rad = Radians(Angle);
        float Sine = Sin(Rad);
        float Cosine = Cos(Rad);

        Axis = Normalize(Axis);

        Result.a00 = Axis.x*Axis.x*(1.0f - Cosine) + Cosine;
	    Result.a10 = Axis.x*Axis.y*(1.0f - Cosine) + Axis.z*Sine;
	    Result.a20 = Axis.x*Axis.z*(1.0f - Cosine) - Axis.y*Sine;
	    Result.a30 = 0.0f;

        Result.a01 = Axis.x*Axis.y*(1.0f - Cosine) - Axis.z*Sine;
	    Result.a11 = Axis.y*Axis.y*(1.0f - Cosine) + Cosine;
	    Result.a21 = Axis.y*Axis.z*(1.0f - Cosine) + Axis.x*Sine;
	    Result.a31 = 0.0f;

        Result.a02 = Axis.x*Axis.z*(1.0f - Cosine) + Axis.y*Sine;
	    Result.a12 = Axis.y*Axis.z*(1.0f - Cosine) - Axis.x*Sine;
	    Result.a22 = Axis.z*Axis.z*(1.0f - Cosine) + Cosine;
	    Result.a32 = 0.0f;

	    Result.a03 = 0.0f;
	    Result.a13 = 0.0f;
	    Result.a23 = 0.0f;
	    Result.a33 = 1.0f;
    }
    else
    {
        Result = Identity();
    }

    return Result;
}

mat4 Mat4(const mat3 &M)
{
    mat4 Result;

    Result.a00 = M.a00;
    Result.a10 = M.a10;
    Result.a20 = M.a20;
    Result.a01 = M.a01;
    Result.a11 = M.a11;
    Result.a21 = M.a21;
    Result.a02 = M.a02;
    Result.a12 = M.a12;
    Result.a22 = M.a22;
    Result.a30 = Result.a31 = Result.a32 = Result.a03 = Result.a13 = Result.a23 = 0.0f;
    Result.a33 = 1.0f;

    return Result;
}

mat4 LookAt(vec3 From, vec3 Target, vec3 UpAxis = Vec3(0.0f, 1.0f, 0.0f))
{
    vec3 Forward = Normalize(From - Target);
    vec3 Right = Normalize(Cross(UpAxis, Forward));
    vec3 Up = Cross(Forward, Right);

    mat4 Result;

    Result.a00 = Right.x;
    Result.a10 = Up.x;
    Result.a20 = Forward.x;
    Result.a30 = 0.0f;

    Result.a01 = Right.y;
    Result.a11 = Up.y;
    Result.a21 = Forward.y;
    Result.a31 = 0.0f;

    Result.a02 = Right.z;
    Result.a12 = Up.z;
    Result.a22 = Forward.z;
    Result.a32 = 0.0f;

    Result.a03 = -Dot(Right, From);
    Result.a13 = -Dot(Up, From);
    Result.a23 = -Dot(Forward, From);
    Result.a33 = 1.0f;

    return Result;
}

mat4 Orthographic(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    mat4 Result = {};

    Result.a00 = 2.0f / (Right - Left);
    Result.a11 = 2.0f / (Top - Bottom);
    Result.a22 = 2.0f / (Far - Near);

    Result.a03 = -(Right + Left) / (Right - Left);
    Result.a13 = -(Top + Bottom) / (Top - Bottom);
	Result.a23 = -(Far + Near) / (Far - Near);
	Result.a33 = 1.0f;

    return Result;
}

mat4 Perspective(float FoV, float AspectRatio, float Near, float Far)
{
    float Scale = Tan(Radians(FoV) * 0.5f) * Near;
    float Top = Scale;
    float Bottom = -Top;
    float Right = Scale * AspectRatio;
    float Left = -Right;

    mat4 Result = {};

    Result.a00 = 2.0f * Near / (Right - Left);
	Result.a11 = 2.0f * Near / (Top - Bottom);
	Result.a02 = (Right + Left) / (Right - Left);
	Result.a12 = (Top + Bottom) / (Top - Bottom);
	Result.a22 = -(Far + Near) / (Far - Near);
	Result.a32 = -1.0f;
	Result.a23 = -(2.0f * Far*Near) / (Far - Near);

    return Result;
}

mat4 Inverse(mat4 M)
{
	mat4 Result = M;
	float Temp[12];

	// Calculate pairs for first 8 elements (cofactors)
	Temp[0] = M.a22 * M.a33;
	Temp[1] = M.a32 * M.a23;
	Temp[2] = M.a12 * M.a33;
	Temp[3] = M.a32 * M.a13;
	Temp[4] = M.a12 * M.a23;
	Temp[5] = M.a22 * M.a13;
	Temp[6] = M.a02 * M.a33;
	Temp[7] = M.a32 * M.a03;
	Temp[8] = M.a02 * M.a23;
	Temp[9] = M.a22 * M.a03;
	Temp[10]= M.a02 * M.a13;
	Temp[11]= M.a12 * M.a03;

	// Calculate first 8 elements (cofactors)
	Result.a00 = Temp[0] * M.a11 + Temp[3] * M.a21 + Temp[ 4] * M.a31;
	Result.a00-= Temp[1] * M.a11 + Temp[2] * M.a21 + Temp[ 5] * M.a31;
	Result.a01 = Temp[1] * M.a01 + Temp[6] * M.a21 + Temp[ 9] * M.a31;
	Result.a01-= Temp[0] * M.a01 + Temp[7] * M.a21 + Temp[ 8] * M.a31;
	Result.a02 = Temp[2] * M.a01 + Temp[7] * M.a11 + Temp[10] * M.a31;
	Result.a02-= Temp[3] * M.a01 + Temp[6] * M.a11 + Temp[11] * M.a31;
	Result.a03 = Temp[5] * M.a01 + Temp[8] * M.a11 + Temp[11] * M.a21;
	Result.a03-= Temp[4] * M.a01 + Temp[9] * M.a11 + Temp[10] * M.a21;
	Result.a10 = Temp[1] * M.a10 + Temp[2] * M.a20 + Temp[ 5] * M.a30;
	Result.a10-= Temp[0] * M.a10 + Temp[3] * M.a20 + Temp[ 4] * M.a30;
	Result.a11 = Temp[0] * M.a00 + Temp[7] * M.a20 + Temp[ 8] * M.a30;
	Result.a11-= Temp[1] * M.a00 + Temp[6] * M.a20 + Temp[ 9] * M.a30;
	Result.a12 = Temp[3] * M.a00 + Temp[6] * M.a10 + Temp[11] * M.a30;
	Result.a12-= Temp[2] * M.a00 + Temp[7] * M.a10 + Temp[10] * M.a30;
	Result.a13 = Temp[4] * M.a00 + Temp[9] * M.a10 + Temp[10] * M.a20;
	Result.a13-= Temp[5] * M.a00 + Temp[8] * M.a10 + Temp[11] * M.a20;

	// Calculate pairs for second 8 elements (cofactors)
	Temp[ 0] = M.a20 * M.a31;
	Temp[ 1] = M.a30 * M.a21;
	Temp[ 2] = M.a10 * M.a31;
	Temp[ 3] = M.a30 * M.a11;
	Temp[ 4] = M.a10 * M.a21;
	Temp[ 5] = M.a20 * M.a11;
	Temp[ 6] = M.a00 * M.a31;
	Temp[ 7] = M.a30 * M.a01;
	Temp[ 8] = M.a00 * M.a21;
	Temp[ 9] = M.a20 * M.a01;
	Temp[10] = M.a00 * M.a11;
	Temp[11] = M.a10 * M.a01;

	// Calculate second 8 elements (cofactors)
	Result.a20 = Temp[ 0] * M.a13 + Temp[ 3] * M.a23 + Temp[ 4] * M.a33;
	Result.a20-= Temp[ 1] * M.a13 + Temp[ 2] * M.a23 + Temp[ 5] * M.a33;
	Result.a21 = Temp[ 1] * M.a03 + Temp[ 6] * M.a23 + Temp[ 9] * M.a33;
	Result.a21-= Temp[ 0] * M.a03 + Temp[ 7] * M.a23 + Temp[ 8] * M.a33;
	Result.a22 = Temp[ 2] * M.a03 + Temp[ 7] * M.a13 + Temp[10] * M.a33;
	Result.a22-= Temp[ 3] * M.a03 + Temp[ 6] * M.a13 + Temp[11] * M.a33;
	Result.a23 = Temp[ 5] * M.a03 + Temp[ 8] * M.a13 + Temp[11] * M.a23;
	Result.a23-= Temp[ 4] * M.a03 + Temp[ 9] * M.a13 + Temp[10] * M.a23;
	Result.a30 = Temp[ 2] * M.a22 + Temp[ 5] * M.a32 + Temp[ 1] * M.a12;
	Result.a30-= Temp[ 4] * M.a32 + Temp[ 0] * M.a12 + Temp[ 3] * M.a22;
	Result.a31 = Temp[ 8] * M.a32 + Temp[ 0] * M.a02 + Temp[ 7] * M.a22;
	Result.a31-= Temp[ 6] * M.a22 + Temp[ 9] * M.a32 + Temp[ 1] * M.a02;
	Result.a32 = Temp[ 6] * M.a12 + Temp[11] * M.a32 + Temp[ 3] * M.a02;
	Result.a32-= Temp[10] * M.a32 + Temp[ 2] * M.a02 + Temp[ 7] * M.a12;
	Result.a33 = Temp[10] * M.a22 + Temp[ 4] * M.a02 + Temp[ 9] * M.a12;
	Result.a33-= Temp[ 8] * M.a12 + Temp[11] * M.a22 + Temp[ 5] * M.a02;

	// Calculate determinant
	float Determinant = (M.a00*Result.a00 + M.a10*Result.a01 + M.a20*Result.a02 + M.a30*Result.a03);
	Assert(Absolute(Determinant) > 0.0001f);

	// Divide the cofactor-matrix by the determinant
	float DeterminantInv = 1.0f / Determinant;
	Result.a00 *= DeterminantInv; Result.a01 *= DeterminantInv; Result.a02 *= DeterminantInv; Result.a03 *= DeterminantInv;
	Result.a10 *= DeterminantInv; Result.a11 *= DeterminantInv; Result.a12 *= DeterminantInv; Result.a13 *= DeterminantInv;
	Result.a20 *= DeterminantInv; Result.a21 *= DeterminantInv; Result.a22 *= DeterminantInv; Result.a23 *= DeterminantInv;
	Result.a30 *= DeterminantInv; Result.a31 *= DeterminantInv; Result.a32 *= DeterminantInv; Result.a33 *= DeterminantInv;

	return Result;
}

mat4 operator*(mat4 A, mat4 B)
{
    mat4 Result;

    for(uint32_t Row = 0; Row < 4; Row++)
    {
        for(uint32_t Column = 0; Column < 4; Column++)
        {
            float Sum = 0.0f;
            for(uint32_t E = 0; E < 4; E++)
            {
                Sum += A.E[Row + E*4] * B.E[Column*4 + E];
            }
            Result.E[Row + Column*4] = Sum;
        }
    }

    return Result;
}

vec4 operator*(mat4 A, vec4 B)
{
	vec4 Result;

	Result.x = A.a00*B.x + A.a01*B.y + A.a02*B.z + A.a03*B.w;
	Result.y = A.a10*B.x + A.a11*B.y + A.a12*B.z + A.a13*B.w;
	Result.z = A.a20*B.x + A.a21*B.y + A.a22*B.z + A.a23*B.w;
	Result.w = A.a30*B.x + A.a31*B.y + A.a32*B.z + A.a33*B.w;

	return Result;
}

//
// NOTE(georgii): Rect
//

inline Rect RectMinMax(vec3 Min, vec3 Max)
{
    Rect Result;

    Result.Min = Min;
    Result.Max = Max;

    return Result;
}

inline Rect RectMinDim(vec3 Min, vec3 Dim)
{
    Rect Result;

    Result.Min = Min;
    Result.Max = Min + Dim;

    return Result;
}

inline Rect RectCenterHalfDim(vec3 Center, vec3 HalfDim)
{
    Rect Result;

    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;

    return Result;
}

inline Rect RectCenterDim(vec3 Center, vec3 Dim)
{
    Rect Result = RectCenterHalfDim(Center, 0.5f*Dim);
    
    return Result;
}

inline Rect RectOffset(Rect Rectangle, vec3 Offset)
{
	Rect Result;

	Result.Min = Rectangle.Min + Offset;
	Result.Max = Rectangle.Max + Offset;

	return Result;
}

inline vec3 GetCenter(Rect Rectangle)
{
    vec3 Result = 0.5f * (Rectangle.Max + Rectangle.Min);

    return Result;
}

inline vec3 GetDim(Rect Rectangle)
{
    vec3 Result = Rectangle.Max - Rectangle.Min;

    return Result;
}

void PointsCenterDimOrientation(vec3* Points, vec3 Center, vec3 Dim, quat Quaternion)
{
    vec3 HalfDim = 0.5f * Dim;

	Points[0] = Center + RotateByQuaternion(Vec3(-HalfDim.x, -HalfDim.y, -HalfDim.z), Quaternion);
    Points[1] = Center + RotateByQuaternion(Vec3(HalfDim.x, -HalfDim.y, -HalfDim.z), Quaternion);
    Points[2] = Center + RotateByQuaternion(Vec3(-HalfDim.x, -HalfDim.y, HalfDim.z), Quaternion);
    Points[3] = Center + RotateByQuaternion(Vec3(HalfDim.x, -HalfDim.y, HalfDim.z), Quaternion);
    Points[4] = Center + RotateByQuaternion(Vec3(-HalfDim.x, HalfDim.y, -HalfDim.z), Quaternion);
    Points[5] = Center + RotateByQuaternion(Vec3(HalfDim.x, HalfDim.y, -HalfDim.z), Quaternion);
    Points[6] = Center + RotateByQuaternion(Vec3(-HalfDim.x, HalfDim.y, HalfDim.z), Quaternion);
    Points[7] = Center + RotateByQuaternion(Vec3(HalfDim.x, HalfDim.y, HalfDim.z), Quaternion);
}

Rect RectCenterDimOrientation(vec3 Center, vec3 Dim, quat Quaternion)
{
    vec3 Points[8];
	PointsCenterDimOrientation(Points, Center, Dim, Quaternion);

    Rect Result = { Vec3(FloatMax), Vec3(-FloatMax) };
    for (uint32_t I = 0; I < ArrayCount(Points); I++)
    {
	    Result.Min.x = Min(Result.Min.x, Points[I].x);
	    Result.Min.y = Min(Result.Min.y, Points[I].y);
	    Result.Min.z = Min(Result.Min.z, Points[I].z);
	    Result.Max.x = Max(Result.Max.x, Points[I].x);
        Result.Max.y = Max(Result.Max.y, Points[I].y);
        Result.Max.z = Max(Result.Max.z, Points[I].z);
    }

    return Result;
}

Rect RectMinDimOrientation(vec3 Min, vec3 Dim, quat Quaternion)
{
	Rect Result = RectCenterDimOrientation(Min + 0.5f * Dim, Dim, Quaternion);
	
	return Result;
}

Rect RectRectOrientation(Rect Rectangle, quat Quaternion)
{
	vec3 Dim = GetDim(Rectangle);

	vec3 Points[8];
    Points[0] = RotateByQuaternion(Rectangle.Min, Quaternion);
    Points[1] = RotateByQuaternion(Rectangle.Min + Vec3(Dim.x, 0.0f, 0.0f), Quaternion);
    Points[2] = RotateByQuaternion(Rectangle.Min + Vec3(0.0f, 0.0f, Dim.z), Quaternion);
    Points[3] = RotateByQuaternion(Rectangle.Min + Vec3(Dim.x, 0.0f, Dim.z), Quaternion);
    Points[4] = RotateByQuaternion(Rectangle.Min + Vec3(0.0f, Dim.y, 0.0f), Quaternion);
    Points[5] = RotateByQuaternion(Rectangle.Min + Vec3(Dim.x, Dim.y, 0.0f), Quaternion);
    Points[6] = RotateByQuaternion(Rectangle.Min + Vec3(0.0f, Dim.y, Dim.z), Quaternion);
    Points[7] = RotateByQuaternion(Rectangle.Min + Vec3(Dim.x, Dim.y, Dim.z), Quaternion);

	Rect Result = { Vec3(FloatMax), Vec3(-FloatMax) };
    for (uint32_t I = 0; I < ArrayCount(Points); I++)
    {
	    Result.Min.x = Min(Result.Min.x, Points[I].x);
	    Result.Min.y = Min(Result.Min.y, Points[I].y);
	    Result.Min.z = Min(Result.Min.z, Points[I].z);
	    Result.Max.x = Max(Result.Max.x, Points[I].x);
        Result.Max.y = Max(Result.Max.y, Points[I].y);
        Result.Max.z = Max(Result.Max.z, Points[I].z);
    }

	return Result;
}

// 
// NOTE(georgii): Intersection tests
//

bool IntersectSphereRay(vec3 Pos, float Radius, vec3 RayStart, vec3 RayDir, float& t)
{
    vec3 CenterToRay = RayStart - Pos;
    float b = Dot(CenterToRay, RayDir);
    float c = Dot(CenterToRay, CenterToRay) - Radius * Radius;

    // If ray's origin outside the sphere (c > 0) and ray points away from the sphere (b > 0)
    if ((c > 0.0f) && (b > 0.0f))
    {
        return false;
    }

    float Discriminant = b * b - c;
    if (Discriminant < 0.0f)
    {
        return false;
    }

    t = -b - SquareRoot(Discriminant);
    if (t < 0.0f)
    {
        t = 0.0f;
    }

    return true;
}

bool IntersectRayTriangle(vec3 RayStart, vec3 RayDir, vec3 A, vec3 B, vec3 C, float& t)
{
	vec3 AB = B - A;
	vec3 AC = C - A;
	vec3 RayDirInv = -RayDir;

	vec3 N = Cross(AB, AC);

	float Denom = Dot(RayDirInv, N);
	if (Absolute(Denom) <= FloatEpsilon)
    {
		return false;
    }

	vec3 RayStartA = RayStart - A;
	t = Dot(RayStartA, N);
	float v = Dot(RayDirInv, Cross(RayStartA, AC));
	float w = Dot(RayDirInv, Cross(AB, RayStartA));
	
	t /= Denom;
    v /= Denom;
    w /= Denom;

    if ((v < 0.0f) || (w < 0.0f) || (v + w > 1.0f))
    {
        return false;
    }

	return true;
}

bool IntersectRectRay(const Rect& AABB, vec3 RayStart, vec3 RayDir, float& t)
{
    float tMin = 0.0f;
    float tMax = FloatMax;

    for (uint32_t I = 0; I < 3; I++)
    {
        if (Absolute(RayDir.E[I]) < FloatEpsilon)
        {
            if ((RayStart.E[I] < AABB.Min.E[I]) || (RayStart.E[I] >= AABB.Max.E[I])) 
            {
                return false;
            }
        }
        else
        {
            float Denom = 1.0f / RayDir.E[I];
            float t1 = (AABB.Min.E[I] - RayStart.E[I]) * Denom;
            float t2 = (AABB.Max.E[I] - RayStart.E[I]) * Denom;

            if (t1 > t2)
            {
                float Temp = t1;
                t1 = t2;
                t2 = Temp;
            }

            if (t1 > tMin) tMin = t1;
            if (t2 < tMax) tMax = t2;

            if (tMin > tMax) 
            {
                return false;
            }
        }
    }

    t = tMin;

    return true;
}

bool IntersectOBBRay(vec3 Origin, vec3 Extents[3], vec3 RayStart, vec3 RayDir, float& t)
{
    float tMin = 0.0f;
    float tMax = FloatMax;

    for (uint32_t I = 0; I < 3; I++)
    {
		float Len = Length(Extents[I]);
		vec3 Extent = Normalize(Extents[I]);
        if (Absolute(Dot(RayDir, Extent)) < FloatEpsilon)
        {
			float Distance = Absolute(Dot(RayStart - Origin, Extent));
            if ((Distance < Len) || (Distance >= Len))
            {
                return false;
            } 
        }
        else
        {
            float Denom = 1.0f / Dot(RayDir, Extent);
            float t1 = Dot((Origin - Extents[I]) - RayStart, Extent) * Denom;
            float t2 = Dot((Origin + Extents[I]) - RayStart, Extent) * Denom;

            if (t1 > t2)
            {
                float Temp = t1;
                t1 = t2;
                t2 = Temp;
            }

            if (t1 > tMin) tMin = t1;
            if (t2 < tMax) tMax = t2;

            if (tMin > tMax) 
            {
                return false;
            }
        }
    }

    t = tMin;

    return true;
}

bool IntersectRects(const Rect& A, const Rect& B)
{
    bool bResult = !((A.Max.x <= B.Min.x || A.Max.y <= B.Min.y || A.Max.z <= B.Min.z) ||
                    (B.Max.x <= A.Min.x || B.Max.y <= A.Min.y || B.Max.z <= A.Min.z));

    return bResult;
}

enum EIntersectMovingRectsResult
{
    Intersect_None,
    Intersect_AtStart,
    Intersect_Moving
};
EIntersectMovingRectsResult IntersectMovingRects(const Rect& A, const Rect& B, vec3 V, float& t, vec3& Normal)
{
    EIntersectMovingRectsResult Result = Intersect_Moving;

    if (IntersectRects(A, B))
    {
        Result = Intersect_AtStart;

        float MinIntersection = FloatMax;
        vec3 IntersectionNormal = Vec3(0.0f);
        for (uint32_t I = 0; I < 3; I++)
        {
            if (A.Max.E[I] > B.Max.E[I])
            {
                float Intersection = B.Max.E[I] - A.Min.E[I];
                Assert(Intersection > 0.0f);

                if (Intersection < MinIntersection)
                {
                    MinIntersection = Intersection;
                    IntersectionNormal = Vec3(0.0f);
                    IntersectionNormal.E[I] = 1.0f;
                }
            }
            else
            {
                float Intersection = A.Max.E[I] - B.Min.E[I];
                Assert(Intersection > 0.0f);

                if (Intersection < MinIntersection)
                {
                    MinIntersection = Intersection;
                    IntersectionNormal = Vec3(0.0f);
                    IntersectionNormal.E[I] = -1.0f;
                }
            }
        }

        t = MinIntersection;
        Normal = IntersectionNormal;
    }
    else
    {
        float tFirst = 0.0f;
        float tLast = 1.0f;

        // For each axis, determine times of first and last contact, if any
        for (uint32_t I = 0; I < 3; I++)
        {
            if (V.E[I] > 0.0f)
            {
                if (A.Min.E[I] >= B.Max.E[I]) { Result = Intersect_None; break; }
                if (A.Max.E[I] <= B.Min.E[I]) 
                { 
                    float tNew = (B.Min.E[I] - A.Max.E[I]) / V.E[I];
                    if (tNew >= tFirst)
                    {
                        tFirst = tNew;
                        Normal = Vec3(0.0f);
                        Normal.E[I] = -1.0f;
                    }
                }
                if (A.Min.E[I] < B.Max.E[I])  
                { 
                    tLast = Min(tLast, (B.Max.E[I] - A.Min.E[I]) / V.E[I]); 
                }
            }
            else if (V.E[I] < 0.0f)
            {
                if (A.Max.E[I] <= B.Min.E[I]) { Result = Intersect_None; break; }
                if (A.Min.E[I] >= B.Max.E[I]) 
                { 
                    float tNew = (A.Min.E[I] - B.Max.E[I]) / -V.E[I];
                    if (tNew >= tFirst)
                    {
                        tFirst = tNew; 
                        Normal = Vec3(0.0f);
                        Normal.E[I] = 1.0f;
                    }
                }
                if (A.Max.E[I] > B.Min.E[I])  
                { 
                    tLast = Min(tLast, (A.Max.E[I] - B.Min.E[I]) / -V.E[I]); 
                }
            }
            else
            {
                Assert(V.E[I] == 0.0f);

                bool Intersect = (A.Max.E[I] > B.Min.E[I]) && (B.Max.E[I] > A.Min.E[I]);
                if (!Intersect) 
                { 
                    Result = Intersect_None;
                    break; 
                }
            }

            if (tFirst > tLast)
            {
                Result = Intersect_None;
                break;
            }
        }

        if (Result == Intersect_Moving) 
        { 
            t = tFirst; 
        }
    }

    return Result;
}

inline bool IsPointInRect(vec2 Point, vec4 Rect)
{
	bool bResult = !(Point.x < Rect.x || Point.x >= Rect.z ||
					 Point.y < Rect.y || Point.y >= Rect.w);
	
	return bResult;
}

//
// NOTE(georgii): Random
//

inline float RandomFloat01()
{
    float Result = (rand() % (RAND_MAX + 1)) / float(RAND_MAX);
    
    return Result;
}

inline uint32_t RandomU32(uint32_t Min, uint32_t Max)
{
    Assert(Max > Min);
    float Step = RandomFloat01() * (Max - Min);
    uint32_t Result = uint32_t(Min + Step + 0.5f);

    Assert((Result >= Min) && (Result <= Max));
    return Result;
}

inline float RandomFloat(float Min, float Max)
{
    float t = RandomFloat01();
    float Result = Lerp(Min, Max, t);

    return Result;
}

inline vec2 RandomVec2(vec2 Min, vec2 Max)
{
    vec2 t = Vec2(RandomFloat01(), RandomFloat01());
    vec2 Result = Lerp(Min, Max, t);

    return Result;
}

inline vec3 RandomVec3(vec3 Min, vec3 Max)
{
    vec3 t = Vec3(RandomFloat01(), RandomFloat01(), RandomFloat01());
    vec3 Result = Lerp(Min, Max, t);

    return Result;
}

inline vec4 RandomVec4(vec4 Min, vec4 Max)
{
    vec4 t = Vec4(RandomFloat01(), RandomFloat01(), RandomFloat01(), RandomFloat01());
    vec4 Result = Lerp(Min, Max, t);

    return Result;
}