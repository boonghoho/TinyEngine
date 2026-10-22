#pragma once

#include "player_controller.h"
#include "../Level/level.h"

#include <array>

namespace tiny
{

class LightingDemo
{
public:
    bool Initialize(ID3D11Device* Device);
    void Update(const Input& GameInput, bool bAcceptGameInput, f32 DeltaSeconds);
    void BuildRenderFrame(const Camera2D& Camera, const RenderSettings& Settings, RenderFrameData& Frame) const;

private:
    static constexpr u32 BouncingLightCount = 2;

    Texture2D MagicatTexture;
    Level GameLevel;
    PlayerController CatController;
    u32 PlayerID = 0;
    std::array<u32, BouncingLightCount> BouncingLightIDs = {};
    std::array<Vec2, BouncingLightCount> BouncingLightVelocities = {};
};

}
