#pragma once

#include "../Core/types.h"
#include "Components/transform2d.h"
#include "Components/sprite.h"

#include <optional>

namespace tiny
{

struct Entity
{
    u32 ID = 0;

	// NOTE(ljh): 현재는 Entity가 Transform2D 컴포넌트를 갖도록 함.
    Transform2D Transform;

    std::optional<Sprite> SpriteComponent;
};

}
