#pragma once

#include "types.h"

namespace tiny
{

struct Vec2
{
    f32 X = 0.0f;
    f32 Y = 0.0f;

    f32 LengthSquared() const
    {
        return X * X + Y * Y;
    }
};

// NOTE(ljh): TinyEngine은 행 벡터와 행 우선 저장 규칙을 사용한다.
struct Mat4
{
    float M[4][4] = {};
};

static_assert(sizeof(Mat4) == sizeof(float) * 16);

struct AABB
{
    Vec2 Min;
    Vec2 Max;

    bool Contains(const Vec2& Point) const
    {
        return Point.X >= Min.X && Point.X <= Max.X &&
               Point.Y >= Min.Y && Point.Y <= Max.Y;
    }

    bool Intersects(const AABB& Other) const
    {
        return Min.X < Other.Max.X &&
               Max.X > Other.Min.X &&
               Min.Y < Other.Max.Y &&
               Max.Y > Other.Min.Y;
    }
};

}
