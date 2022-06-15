#pragma once

#include <math.h>
#include <intrin.h>

inline float Square(float Value)
{
    float Result = Value*Value;
    
    return Result;
}

inline float SquareRoot(float Value)
{
    float Result = sqrtf(Value);

    return Result;
}

inline float Sign(float Value)
{
    float Result;

    if (Value > 0.0f) 
    {
        Result = 1.0f;
    }
    else if (Value < 0.0f)
    {
        Result = -1.0f;
    }
    else 
    {
        Result = 0.0f;
    }

    return Result;
}

inline int32_t Sign(int32_t Value)
{
    int32_t Result;

    if (Value > 0) 
    {
        Result = 1;
    }
    else if (Value < 0)
    {
        Result = -1;
    }
    else 
    {
        Result = 0;
    }

    return Result;
}

inline float Floor(float Value)
{
    float Result = (float)((int32_t)Value);
    if (Value < 0)
    {
        Result -= 1.0f;
    } 

    return Result;
}

inline uint32_t FloorToUInt32(float Value)
{
	uint32_t Result = (uint32_t)Value;

    return Result;
}

inline uint32_t CeilToUInt32(float Value)
{
	uint32_t Result = (uint32_t)Value;
	if (Value > Result) 
    {
        Result += 1; 
    }

    return Result;
}

inline float Absolute(float Value)
{
    int ResultInt = (*(int*)&Value) & 0x7fffffff;
    float Result = *(float*)&ResultInt;

    return Result;
}

inline int32_t Absolute(int32_t Value)
{
    uint32_t Result; 
    if (Value >= 0)
    {
        Result = Value;
    }
    else
    {
        Result = -Value;
    }

    return Result;
}

inline float Sin(float Angle)
{
    float Result = sinf(Angle);

    return Result;
}

inline float Cos(float Angle)
{
    float Result = cosf(Angle);

    return Result;
}

inline float Tan(float Angle)
{
    float Result = tanf(Angle);

    return Result;
}

inline float ArcCos(float Value)
{
    float Result = acosf(Value);

    return Result;
}

inline float Log2(float Value)
{
    float Result = log2f(Value);

    return Result;
}

inline float SafeDivide0(float Dividend, float Divisor)
{
    float Result;

    if (Divisor == 0.0f)
    {
        Result = 0.0f;
    }
    else
    {
        Result = Dividend / Divisor;
    }

    return Result;
}

inline uint32_t FindLeastSignificantSetBitIndex(uint32_t Value)
{
    uint32_t Result = 0;

    _BitScanForward((unsigned long *)&Result, Value);

    return Result;
}

inline bool IsPowerOfTwo(uint32_t Value)
{
    bool bResult = ((Value & (Value - 1)) == 0) && (Value > 0);

    return bResult;
}

uint32_t NextOrEqualPowerOfTwo(uint32_t Value)
{
    uint32_t Result = Value;
    if (!IsPowerOfTwo(Value))
    {
        uint32_t Temp = 1;
        while (Temp < Value) Temp *= 2;
        Result = Temp;
    }

    return Result;
}

inline bool IsDigit(char Char)
{
    bool bResult = (Char >= '0') && (Char <= '9');

    return bResult;
}

uint32_t StringLength(const char* Str)
{
    uint32_t Length = 0;
    for (const char* C = Str; *C; C++)
    {
        Length++;
    }

    return Length;
}

bool CompareStrings(const char* A, uint32_t ALength, const char* B)
{
    bool bResult = true;

    while (ALength && *B)
    {
        if (*A != *B)
            break;

        A++;
        ALength--;
        
        B++;
    }

    if (((ALength > 0) && (*B == '\0')) || (*B != '\0'))
    {
       bResult = false;
    }

    return bResult;
}

bool CompareStrings(const char* A, const char* B)
{
	bool bResult = CompareStrings(A, StringLength(A), B);

	return bResult;
}

void ConcStrings(char* Buffer, uint32_t BufferSize, const char* A, const char* B, uint32_t BLength)
{
	Assert((StringLength(A) + BLength) <= BufferSize);

    uint32_t Index = 0;
    while (*A)
    {
        Assert(Index < BufferSize);
        Buffer[Index++] = *A;
        A++;
    }

    Assert((Index + BLength) < (BufferSize - 1));
    for (uint32_t I = 0; I < BLength; I++)
    {
        Buffer[Index++] = *B;
        B++;
    }

    Buffer[Index] = 0;
}

void ConcStrings(char* Buffer, uint32_t BufferSize, const char* A, const char* B)
{
    ConcStrings(Buffer, BufferSize, A, B, StringLength(B));
}

int32_t StrToInt32(const char* String, uint32_t Length)
{
    int Result = 0;

    int32_t Sign = 1;
    if (*String == '-')
    {
        Sign = -1;
        String++;
    }
    else if (*String == '+')
    {
        String++;
    }

    while (Length-- && IsDigit(*String))
    {
        Result = 10*Result + (*String - '0');
        String++;
    }

    Result *= Sign;

    return Result;
}

float StrToFloat(const char* String, uint32_t Length)
{
    float Result = 0.0f;
    
    float Mult = 1.0f;
    if (*String == '-')
    {
        Mult = -1.0f;
        String++;
    }
    else if (*String == '+')
    {
        String++;
    }

    bool bPointSeen = false;
    while (Length-- && (IsDigit(*String) || (*String == '.')))
    {
        if (*String == '.')
        {
            bPointSeen = true;
            String++;
            continue;
        }

        Result = 10*Result + (*String - '0');
        if (bPointSeen)
        {
            Mult /= 10.0f;
        }

        String++;
    }

    Result *= Mult;

    return Result;
}

inline uint64_t GetProcessorTimeStamp()
{
    uint64_t Result = __rdtsc();

    return Result;
}