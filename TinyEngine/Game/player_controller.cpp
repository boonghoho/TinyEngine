#include "player_controller.h"

#include "../Core/math_types.h"
#include "../Input/input.h"
#include "../Level/entity.h"

#include <cmath>

namespace tiny
{

namespace
{

AABB GetPlayerBounds(const Entity& Player)
{
    if (!Player.SpriteComponent)
    {
        const Vec2 Position = {Player.Transform.X, Player.Transform.Y};
        return {Position, Position};
    }

    const Sprite& PlayerSprite = *Player.SpriteComponent;
    const f32 Width = PlayerSprite.Width * Player.Transform.ScaleX;
    const f32 Height = PlayerSprite.Height * Player.Transform.ScaleY;

    return {
        {Player.Transform.X, Player.Transform.Y},
        {Player.Transform.X + Width, Player.Transform.Y + Height},
    };
}

void MoveAlongX(Entity& Player, f32 DeltaX, const std::vector<AABB>& Colliders)
{
    Player.Transform.X += DeltaX;

    for (const AABB& Collider : Colliders)
    {
        const AABB PlayerBounds = GetPlayerBounds(Player);
        if (!PlayerBounds.Intersects(Collider))
        {
            continue;
        }

        if (DeltaX > 0.0f)
        {
            Player.Transform.X += Collider.Min.X - PlayerBounds.Max.X;
        }
        else if (DeltaX < 0.0f)
        {
            Player.Transform.X += Collider.Max.X - PlayerBounds.Min.X;
        }
    }
}

void MoveAlongY(Entity& Player, f32 DeltaY, const std::vector<AABB>& Colliders)
{
    Player.Transform.Y += DeltaY;

    for (const AABB& Collider : Colliders)
    {
        const AABB PlayerBounds = GetPlayerBounds(Player);
        if (!PlayerBounds.Intersects(Collider))
        {
            continue;
        }

        if (DeltaY > 0.0f)
        {
            Player.Transform.Y += Collider.Min.Y - PlayerBounds.Max.Y;
        }
        else if (DeltaY < 0.0f)
        {
            Player.Transform.Y += Collider.Max.Y - PlayerBounds.Min.Y;
        }
    }
}

}

void PlayerController::Update(Entity& Player, const Input& GameInput, f32 DeltaSeconds, const std::vector<AABB>& Colliders) const
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

    const f32 DeltaX = MoveDirection.X * MoveSpeed * DeltaSeconds;
    const f32 DeltaY = MoveDirection.Y * MoveSpeed * DeltaSeconds;

    // NOTE(ljh): 축별로 충돌을 해결하면 막히지 않은 축의 이동이 남아 wall을 따라 움직일 수 있다.
    MoveAlongX(Player, DeltaX, Colliders);
    MoveAlongY(Player, DeltaY, Colliders);
}

}
