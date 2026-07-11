#include "radiance_cascade_renderer.h"

#include "d3d11_device.h"

namespace
{
    int CeilDivide(int Value, int Divisor)
    {
        return (Value + Divisor - 1) / Divisor;
    }
}

RadianceCascadeRenderer::~RadianceCascadeRenderer()
{
    Release();
}

bool RadianceCascadeRenderer::Initialize(ID3D11Device* Device, int InSceneWidth, int InSceneHeight)
{
    Release();

    if (!Device || InSceneWidth <= 0 || InSceneHeight <= 0)
    {
        return false;
    }

    SceneWidth = InSceneWidth;
    SceneHeight = InSceneHeight;

    if (!GatherPass.Initialize(Device, L"Shaders/rc_gather.hlsl"))
    {
        Release();
        return false;
    }

    if (!MergePass.Initialize(Device, L"Shaders/rc_merge.hlsl"))
    {
        Release();
        return false;
    }

    if (!CompositePass.Initialize(Device, L"Shaders/rc_composite.hlsl"))
    {
        Release();
        return false;
    }

    for (int CascadeIndex = 0; CascadeIndex < CascadeCount; ++CascadeIndex)
    {
        if (!CascadeTextures[CascadeIndex].Initialize(
                Device,
                GetCascadeTextureWidth(CascadeIndex),
                GetCascadeTextureHeight(CascadeIndex)))
        {
            Release();
            return false;
        }
    }

    for (int CascadeIndex = 0; CascadeIndex < CascadeCount - 1; ++CascadeIndex)
    {
        if (!MergedCascadeTextures[CascadeIndex].Initialize(
                Device,
                GetCascadeTextureWidth(CascadeIndex),
                GetCascadeTextureHeight(CascadeIndex)))
        {
            Release();
            return false;
        }
    }

    return true;
}

void RadianceCascadeRenderer::Release()
{
    for (RenderTexture& Texture : MergedCascadeTextures)
    {
        Texture.Release();
    }

    for (RenderTexture& Texture : CascadeTextures)
    {
        Texture.Release();
    }

    CompositePass.Release();
    MergePass.Release();
    GatherPass.Release();

    SceneWidth = 0;
    SceneHeight = 0;
}

void RadianceCascadeRenderer::GenerateLighting(
    D3D11Device& GraphicsDevice,
    ID3D11ShaderResourceView* SceneShaderResourceView
)
{
    const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    for (int CascadeIndex = 0; CascadeIndex < CascadeCount; ++CascadeIndex)
    {
        RenderTexture& CascadeTexture = CascadeTextures[CascadeIndex];

        GraphicsDevice.SetRenderTarget(
            CascadeTexture.GetRenderTargetView(),
            CascadeTexture.GetWidth(),
            CascadeTexture.GetHeight()
        );
        GraphicsDevice.ClearRenderTarget(CascadeTexture.GetRenderTargetView(), ClearColor);

        ID3D11ShaderResourceView* GatherInputs[] =
        {
            SceneShaderResourceView
        };

        GatherPass.Render(
            GraphicsDevice.GetContext(),
            static_cast<float>(CascadeTexture.GetWidth()),
            static_cast<float>(CascadeTexture.GetHeight()),
            0.0f,
            0.0f,
            0.0f,
            GatherInputs,
            1,
            static_cast<float>(CascadeIndex),
            static_cast<float>(SceneWidth),
            static_cast<float>(SceneHeight)
        );
    }

    for (int CascadeIndex = CascadeCount - 2; CascadeIndex >= 0; --CascadeIndex)
    {
        RenderTexture& MergeTarget = MergedCascadeTextures[CascadeIndex];
        ID3D11ShaderResourceView* FarCascade =
            CascadeIndex == CascadeCount - 2
            ? CascadeTextures[CascadeIndex + 1].GetShaderResourceView()
            : MergedCascadeTextures[CascadeIndex + 1].GetShaderResourceView();

        GraphicsDevice.SetRenderTarget(
            MergeTarget.GetRenderTargetView(),
            MergeTarget.GetWidth(),
            MergeTarget.GetHeight()
        );
        GraphicsDevice.ClearRenderTarget(MergeTarget.GetRenderTargetView(), ClearColor);

        ID3D11ShaderResourceView* MergeInputs[] =
        {
            FarCascade,
            SceneShaderResourceView
        };

        MergePass.Render(
            GraphicsDevice.GetContext(),
            static_cast<float>(MergeTarget.GetWidth()),
            static_cast<float>(MergeTarget.GetHeight()),
            0.0f,
            0.0f,
            0.0f,
            MergeInputs,
            2,
            static_cast<float>(CascadeIndex),
            static_cast<float>(SceneWidth),
            static_cast<float>(SceneHeight)
        );
    }
}

void RadianceCascadeRenderer::DrawFinal(
    ID3D11DeviceContext* DeviceContext,
    ID3D11ShaderResourceView* SceneShaderResourceView,
    int OutputWidth,
    int OutputHeight,
    float IndirectStrength,
    float Exposure
)
{
    ID3D11ShaderResourceView* CompositeInputs[] =
    {
        SceneShaderResourceView,
        MergedCascadeTextures[0].GetShaderResourceView()
    };

    CompositePass.Render(
        DeviceContext,
        static_cast<float>(OutputWidth),
        static_cast<float>(OutputHeight),
        0.0f,
        0.0f,
        0.0f,
        CompositeInputs,
        2,
        IndirectStrength,
        Exposure
    );
}

ID3D11ShaderResourceView* RadianceCascadeRenderer::GetCascadeShaderResourceView(int CascadeIndex) const
{
    if (CascadeIndex < 0 || CascadeIndex >= CascadeCount)
    {
        return nullptr;
    }

    return CascadeTextures[CascadeIndex].GetShaderResourceView();
}

ID3D11ShaderResourceView* RadianceCascadeRenderer::GetMergedShaderResourceView() const
{
    return MergedCascadeTextures[0].GetShaderResourceView();
}

int RadianceCascadeRenderer::GetCascadeTextureWidth(int CascadeIndex) const
{
    const int ProbeSpacingPixels = BaseProbeSpacingPixels << CascadeIndex;
    const int RaySide = BaseRaySide << CascadeIndex;
    return CeilDivide(SceneWidth, ProbeSpacingPixels) * RaySide;
}

int RadianceCascadeRenderer::GetCascadeTextureHeight(int CascadeIndex) const
{
    const int ProbeSpacingPixels = BaseProbeSpacingPixels << CascadeIndex;
    const int RaySide = BaseRaySide << CascadeIndex;
    return CeilDivide(SceneHeight, ProbeSpacingPixels) * RaySide;
}
