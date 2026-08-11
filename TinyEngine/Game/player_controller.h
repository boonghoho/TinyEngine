#pragma once

#include "../Core/types.h"

namespace tiny
{

class Input;
struct Entity;

class PlayerController
{
public:
    void Update(Entity& Player, const Input& GameInput, f32 DeltaSeconds) const;

private:
    f32 MoveSpeed = 250.0f;
};

}
