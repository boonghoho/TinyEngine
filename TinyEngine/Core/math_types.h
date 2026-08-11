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

}
