#pragma once

#include "../../Core/types.h"

namespace tiny
{

class Texture2D;

struct Sprite
{
    Texture2D* Texture = nullptr;
    f32 Width = 0.0f;
    f32 Height = 0.0f;
};

}
