#include "lighting_demo.h"

namespace tiny
{
namespace
{

// NOTE(ljh): RC demo를 위해 임의로 만든 Bouncing Light 설정 구조체와 관련 함수들
struct BouncingLightConfig
{
    Vec2 Position;
    Vec2 Velocity;
    Light2D Light;
};

constexpr std::array<BouncingLightConfig, 2> BouncingLightConfigs = {{
    {{160.0f, 160.0f}, {220.0f, 150.0f}, {0.0f, 1.0f, 0.0f, 25.0f, 12.0f}},
    {{1120.0f, 560.0f}, {-210.0f, -170.0f}, {0.0f, 0.0f, 1.0f, 25.0f, 12.0f}},
}};

void ReflectVelocity(Vec2& Velocity, const Vec2& SurfaceNormal)
{
    const f32 VelocityAlongNormal = Velocity.X * SurfaceNormal.X + Velocity.Y * SurfaceNormal.Y;

    Velocity.X -= 2.0f * VelocityAlongNormal * SurfaceNormal.X;
    Velocity.Y -= 2.0f * VelocityAlongNormal * SurfaceNormal.Y;
}

AABB MakeLightBounds(const Entity& LightEntity, f32 Radius)
{
    return {
        {LightEntity.Transform.X - Radius, LightEntity.Transform.Y - Radius},
        {LightEntity.Transform.X + Radius, LightEntity.Transform.Y + Radius},
    };
}

void UpdateBouncingLight(
    Entity& LightEntity,
    Vec2& Velocity,
    f32 DeltaSeconds,
    const std::vector<AABB>& Colliders)
{
    if (!LightEntity.LightComponent)
    {
        return;
    }

    const f32 Radius = LightEntity.LightComponent->Radius;

    LightEntity.Transform.X += Velocity.X * DeltaSeconds;

    for (const AABB& Collider : Colliders)
    {
        if (!MakeLightBounds(LightEntity, Radius).Intersects(Collider))
        {
            continue;
        }

        if (Velocity.X > 0.0f)
        {
            LightEntity.Transform.X = Collider.Min.X - Radius;
            ReflectVelocity(Velocity, {-1.0f, 0.0f});
        }
        else if (Velocity.X < 0.0f)
        {
            LightEntity.Transform.X = Collider.Max.X + Radius;
            ReflectVelocity(Velocity, {1.0f, 0.0f});
        }

        break;
    }

    LightEntity.Transform.Y += Velocity.Y * DeltaSeconds;

    for (const AABB& Collider : Colliders)
    {
        if (!MakeLightBounds(LightEntity, Radius).Intersects(Collider))
        {
            continue;
        }

        if (Velocity.Y > 0.0f)
        {
            LightEntity.Transform.Y = Collider.Min.Y - Radius;
            ReflectVelocity(Velocity, {0.0f, -1.0f});
        }
        else if (Velocity.Y < 0.0f)
        {
            LightEntity.Transform.Y = Collider.Max.Y + Radius;
            ReflectVelocity(Velocity, {0.0f, 1.0f});
        }

        break;
    }
}

}

bool LightingDemo::Initialize(ID3D11Device* Device)
{
    // NOTE(ljh): 현재는 내가 예전에 만든 Magicat 이미지를 임시로 사용한다.
    if (!MagicatTexture.LoadFromFile(Device, "Assets/magicat.png"))
    {
        return false;
    }

    // NOTE(ljh): 현재 tiled 로 만든 임의의 맵을 불러온다.
    if (!GameLevel.Initialize(Device, "Assets/Maps/dungeon_001.json"))
    {
        return false;
    }

    Entity& Cat = GameLevel.CreateEntity();
    PlayerID = Cat.ID;
    Cat.Transform.X = 500.0f;
    Cat.Transform.Y = 100.0f;
    Cat.Transform.ScaleX = 1.0f;
    Cat.Transform.ScaleY = 1.0f;
    Cat.SpriteComponent.emplace();
    Cat.SpriteComponent->Texture = &MagicatTexture;
    Cat.SpriteComponent->Width = 75.0f;
    Cat.SpriteComponent->Height = 75.0f;

    static_assert(BouncingLightCount == BouncingLightConfigs.size());
    for (u32 LightIndex = 0; LightIndex < BouncingLightCount; ++LightIndex)
    {
        const BouncingLightConfig& Config = BouncingLightConfigs[LightIndex];
        Entity& BouncingLight = GameLevel.CreateEntity();
        BouncingLightIDs[LightIndex] = BouncingLight.ID;
        BouncingLight.Transform.X = Config.Position.X;
        BouncingLight.Transform.Y = Config.Position.Y;
        BouncingLight.LightComponent = Config.Light;
        BouncingLightVelocities[LightIndex] = Config.Velocity;
    }

    return true;
}

void LightingDemo::Update(const Input& GameInput, bool bAcceptGameInput, f32 DeltaSeconds)
{
    // 창 이동 등으로 오래 멈춘 뒤 한 번에 충돌체를 뛰어넘지 않도록 제한한다.
    const f32 SafeDeltaSeconds = DeltaSeconds < (1.0f / 30.0f) ? DeltaSeconds : (1.0f / 30.0f);

    if (bAcceptGameInput)
    {
        if (Entity* Player = GameLevel.GetEntityByID(PlayerID))
        {
            CatController.Update(*Player, GameInput, SafeDeltaSeconds, GameLevel.GetColliders());
        }
    }

    for (u32 LightIndex = 0; LightIndex < BouncingLightCount; ++LightIndex)
    {
        if (Entity* CurrentLight = GameLevel.GetEntityByID(BouncingLightIDs[LightIndex]))
        {
            UpdateBouncingLight(
                *CurrentLight,
                BouncingLightVelocities[LightIndex],
                SafeDeltaSeconds,
                GameLevel.GetColliders());
        }
    }
}

void LightingDemo::BuildRenderFrame(
    const Camera2D& Camera,
    const RenderSettings& Settings,
    RenderFrameData& Frame) const
{
    GameLevel.BuildRenderFrame(Camera, Settings, Frame);
}

}
