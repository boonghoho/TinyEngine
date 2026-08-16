#pragma once

#include "../Core/types.h"
#include "fullscreen_pass.h"
#include "render_texture.h"

#include <array>
#include <d3d11.h>
#include <wrl/client.h>

namespace tiny
{

class D3D11Device;
class Level;

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

    void GenerateLighting(D3D11Device& GraphicsDevice, const Level& GameLevel);

    void ResolveLighting(
        ID3D11DeviceContext* DeviceContext,
        int OutputWidth,
        int OutputHeight,
        float AmbientStrength,
        float IndirectStrength);

    ID3D11ShaderResourceView* GetCascadeShaderResourceView(int CascadeIndex) const;
    ID3D11ShaderResourceView* GetMergedShaderResourceView() const;

private:
    static constexpr int BaseProbeSpacingPixels = 4;
    static constexpr int BaseRaySide = 8;
    static constexpr u32 MaxScenePrimitives = 256;

    // NOTE(ljh): Light와 Collider를 같은 buffer에 담기 위한 struct
    struct ScenePrimitiveGpu
    {
        f32 Shape[4];

        // NOTE(ljh): rgb = Light2D RGB * Intensity, a = Light면 1, Collider면 0.
        f32 LightData[4];
    };

    int GetCascadeTextureWidth(int CascadeIndex) const;
    int GetCascadeTextureHeight(int CascadeIndex) const;
    bool InitializeScenePrimitiveBuffer(ID3D11Device* Device);
    u32 UploadScenePrimitives(ID3D11DeviceContext* DeviceContext, const Level& GameLevel);

    int SceneWidth = 0;
    int SceneHeight = 0;

    FullscreenPass GatherPass;
    FullscreenPass MergePass;
    FullscreenPass ResolvePass;

    std::array<RenderTexture, CascadeCount> CascadeTextures;
    std::array<RenderTexture, CascadeCount - 1> MergedCascadeTextures;

    Microsoft::WRL::ComPtr<ID3D11Buffer> ScenePrimitiveBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ScenePrimitiveShaderResourceView;
};

}
