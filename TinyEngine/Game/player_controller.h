#pragma once

#include "../Core/math_types.h"

#include <vector>

namespace tiny
{

class Input;
struct Entity;

class PlayerController
{
public:
    // NOTE(ljh): 현재 플레이어 컨트롤러에서는 이동시 충돌을 직접 처리한다. 
    // 향후 PhysicsSystem을 구현하면 PlayerController는 이동 입력만 처리하고 PhysicsSystem이 충돌을 처리하도록 변경할 수 있다.
    void Update(Entity& Player, const Input& GameInput, f32 DeltaSeconds, const std::vector<AABB>& Colliders) const;

private:
    f32 MoveSpeed = 250.0f;
};

}
