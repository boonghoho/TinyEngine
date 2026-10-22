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
struct RenderFrameData;

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
        const RenderFrameData& Frame,
        ID3D11Buffer* CameraConstantBuffer);

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
    static constexpr int BaseDirectionTileWidth = 8;
    static constexpr int BaseDirectionTileHeight = 8;
    static constexpr u32 MaxScenePrimitives = 256;

    // NOTE(ljh): 광원과 차폐물을 같은 buffer에 담는다.
    struct ScenePrimitiveGpu
    {
        f32 Shape[4];

        // NOTE(ljh): rgb = 밝기가 반영된 광원 색상, a = 광원이면 1, 차폐물이면 0.
        f32 LightData[4];
    };

    int GetCascadeTextureWidth(int CascadeIndex) const;
    int GetCascadeTextureHeight(int CascadeIndex) const;
    int GetDirectionTileWidth(int CascadeIndex) const;
    int GetDirectionTileHeight(int CascadeIndex) const;
    bool InitializeScenePrimitiveBuffer(ID3D11Device* Device);
    u32 UploadScenePrimitives(ID3D11DeviceContext* DeviceContext, const RenderFrameData& Frame);

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
