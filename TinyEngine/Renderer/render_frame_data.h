#pragma once

#include "camera2d.h"

#include <vector>

struct ID3D11ShaderResourceView;

namespace tiny
{

struct UVRect
{
    float U0 = 0.0f;
    float V0 = 0.0f;
    float U1 = 1.0f;
    float V1 = 1.0f;
};

struct ColorTint
{
    float R = 1.0f;
    float G = 1.0f;
    float B = 1.0f;
    float A = 1.0f;
};

struct SpriteDrawCommand
{
    // 빌린 GPU 리소스 참조다. 소유자는 프레임 소비가 끝날 때까지 텍스처를 유지한다.
    ID3D11ShaderResourceView* TextureSRV = nullptr;

    float X = 0.0f;
    float Y = 0.0f;
    float Width = 0.0f;
    float Height = 0.0f;

    UVRect SourceUV;
    ColorTint Tint;
};

struct RenderLight
{
    float X = 0.0f;
    float Y = 0.0f;
    float Radius = 1.0f;
    float R = 0.0f;
    float G = 0.0f;
    float B = 0.0f;
};

enum class LightingMode
{
    Unlit,
    RadianceCascades,
};

struct RenderSettings
{
    LightingMode Lighting = LightingMode::RadianceCascades;
};

struct RenderFrameData
{
    Camera2D Camera;
    RenderSettings Settings;
    std::vector<SpriteDrawCommand> Sprites;
    std::vector<RenderLight> Lights;
    std::vector<AABB> Occluders;
};

}
