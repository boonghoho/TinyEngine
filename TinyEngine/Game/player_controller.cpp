#include "player_controller.h"

#include "../Core/math_types.h"
#include "../Input/input.h"
#include "../Level/entity.h"

#include <cmath>

namespace tiny
{

void PlayerController::Update(Entity& Player, const Input& GameInput, f32 DeltaSeconds) const
{
    Vec2 MoveDirection;

    if (GameInput.IsDown(SDL_SCANCODE_A))
    {
        MoveDirection.X -= 1.0f;
    }

    if (GameInput.IsDown(SDL_SCANCODE_D))
    {
        MoveDirection.X += 1.0f;
    }

    if (GameInput.IsDown(SDL_SCANCODE_W))
    {
        MoveDirection.Y -= 1.0f;
    }

    if (GameInput.IsDown(SDL_SCANCODE_S))
    {
        MoveDirection.Y += 1.0f;
    }

    const f32 MoveLengthSquared = MoveDirection.LengthSquared();
    if (MoveLengthSquared > 1.0f)
    {
        const f32 InverseLength = 1.0f / std::sqrt(MoveLengthSquared);
        MoveDirection.X *= InverseLength;
        MoveDirection.Y *= InverseLength;
    }

    Player.Transform.X += MoveDirection.X * MoveSpeed * DeltaSeconds;
    Player.Transform.Y += MoveDirection.Y * MoveSpeed * DeltaSeconds;
}

}
