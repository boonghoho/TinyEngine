#pragma once

#include "fullscreen_pass.h"
#include "render_texture.h"

#include <array>

class D3D11Device;

// NOTE(ljh): Radiance Cascades에 필요한 Pass와 중간 Texture를 소유하고 실행한다.
class RadianceCascadeRenderer
{
public:
    static constexpr int CascadeCount = 4;

    RadianceCascadeRenderer() = default;
    ~RadianceCascadeRenderer();

    RadianceCascadeRenderer(const RadianceCascadeRenderer&) = delete;
    RadianceCascadeRenderer& operator=(const RadianceCascadeRenderer&) = delete;

    bool Initialize(ID3D11Device* Device, int SceneWidth, int SceneHeight);
    void Release();

    void GenerateLighting(
        D3D11Device& GraphicsDevice,
        ID3D11ShaderResourceView* SceneShaderResourceView
    );

    void DrawFinal(
        ID3D11DeviceContext* DeviceContext,
        ID3D11ShaderResourceView* SceneShaderResourceView,
        int OutputWidth,
        int OutputHeight,
        float IndirectStrength,
        float Exposure
    );

    ID3D11ShaderResourceView* GetCascadeShaderResourceView(int CascadeIndex) const;
    ID3D11ShaderResourceView* GetMergedShaderResourceView() const;

private:
    static constexpr int BaseProbeSpacingPixels = 16;
    static constexpr int BaseRaySide = 4;

    int GetCascadeTextureWidth(int CascadeIndex) const;
    int GetCascadeTextureHeight(int CascadeIndex) const;

    int SceneWidth = 0;
    int SceneHeight = 0;

    FullscreenPass GatherPass;
    FullscreenPass MergePass;
    FullscreenPass CompositePass;

    std::array<RenderTexture, CascadeCount> CascadeTextures;
    std::array<RenderTexture, CascadeCount - 1> MergedCascadeTextures;
};
