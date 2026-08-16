#include "radiance_cascade_renderer.h"

#include "d3d11_device.h"
#include "../Level/level.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace tiny
{

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

    if (!ResolvePass.Initialize(Device, L"Shaders/rc_resolve.hlsl"))
    {
        Release();
        return false;
    }

    if (!InitializeScenePrimitiveBuffer(Device))
    {
        Release();
        return false;
    }

    // NOTE(ljh): 한 probe가 RaySide x RaySide texel 블록을 소유하는 packed cascade texture를 만든다.
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

    // NOTE(ljh): C3 -> C2 -> C1 -> C0 순서로 누적할 때 필요한 중간 결과 texture를 만든다.
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

    ScenePrimitiveShaderResourceView.Reset();
    ScenePrimitiveBuffer.Reset();

    ResolvePass.Release();
    MergePass.Release();
    GatherPass.Release();

    SceneWidth = 0;
    SceneHeight = 0;
}

void RadianceCascadeRenderer::GenerateLighting(D3D11Device& GraphicsDevice, const Level& GameLevel)
{
    const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    // NOTE(ljh): 움직이는 light도 매 frame 반영되도록 collider와 Light2D를 GPU structured buffer에 다시 올린다.
    const u32 ScenePrimitiveCount = UploadScenePrimitives(GraphicsDevice.GetContext(), GameLevel);

    static const wchar_t* GatherEventNames[CascadeCount] = {
        L"RC Gather C0",
        L"RC Gather C1",
        L"RC Gather C2",
        L"RC Gather C3"};

    for (int CascadeIndex = 0; CascadeIndex < CascadeCount; ++CascadeIndex)
    {
        GraphicsDevice.BeginGpuEvent(GatherEventNames[CascadeIndex]);

        RenderTexture& CascadeTexture = CascadeTextures[CascadeIndex];

        GraphicsDevice.SetRenderTarget(
            CascadeTexture.GetRenderTargetView(),
            CascadeTexture.GetWidth(),
            CascadeTexture.GetHeight());
        GraphicsDevice.ClearRenderTarget(CascadeTexture.GetRenderTargetView(), ClearColor);

        ID3D11ShaderResourceView* GatherInputs[] = {ScenePrimitiveShaderResourceView.Get()};

        GatherPass.Render(
            GraphicsDevice.GetContext(),
            static_cast<float>(CascadeTexture.GetWidth()),
            static_cast<float>(CascadeTexture.GetHeight()),
            GatherInputs,
            1,
            static_cast<float>(CascadeIndex),
            static_cast<float>(ScenePrimitiveCount));

        GraphicsDevice.EndGpuEvent();
    }

    static const wchar_t* MergeEventNames[CascadeCount - 1] =
        {
            L"RC Merge C1 into C0",
            L"RC Merge C2 into C1",
            L"RC Merge C3 into C2"};

    // NOTE(ljh): C3 부터 C0까지 cascade를 합쳐서 C0에 최종 결과를 만든다.
    for (int CascadeIndex = CascadeCount - 2; CascadeIndex >= 0; --CascadeIndex)
    {
        GraphicsDevice.BeginGpuEvent(MergeEventNames[CascadeIndex]);

        RenderTexture& MergeTarget = MergedCascadeTextures[CascadeIndex];
        ID3D11ShaderResourceView* FarCascade =
            CascadeIndex == CascadeCount - 2
                ? CascadeTextures[CascadeIndex + 1].GetShaderResourceView()
                : MergedCascadeTextures[CascadeIndex + 1].GetShaderResourceView();

        GraphicsDevice.SetRenderTarget(
            MergeTarget.GetRenderTargetView(),
            MergeTarget.GetWidth(),
            MergeTarget.GetHeight());
        GraphicsDevice.ClearRenderTarget(MergeTarget.GetRenderTargetView(), ClearColor);

        ID3D11ShaderResourceView* MergeInputs[] = {CascadeTextures[CascadeIndex].GetShaderResourceView(), FarCascade};

        MergePass.Render(
            GraphicsDevice.GetContext(),
            static_cast<float>(MergeTarget.GetWidth()),
            static_cast<float>(MergeTarget.GetHeight()),
            MergeInputs,
            2,
            static_cast<float>(CascadeIndex));

        GraphicsDevice.EndGpuEvent();
    }
}

void RadianceCascadeRenderer::ResolveLighting(
    ID3D11DeviceContext* DeviceContext,
    int OutputWidth,
    int OutputHeight,
    float AmbientStrength,
    float IndirectStrength)
{
    ID3D11ShaderResourceView* ResolveInputs[] = {MergedCascadeTextures[0].GetShaderResourceView()};

    ResolvePass.Render(
        DeviceContext,
        static_cast<float>(OutputWidth),
        static_cast<float>(OutputHeight),
        ResolveInputs,
        1,
        AmbientStrength,
        IndirectStrength);
}

bool RadianceCascadeRenderer::InitializeScenePrimitiveBuffer(ID3D11Device* Device)
{
    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.ByteWidth = sizeof(ScenePrimitiveGpu) * MaxScenePrimitives;
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(ScenePrimitiveGpu);

    HRESULT Result = Device->CreateBuffer(
        &BufferDesc,
        nullptr,
        ScenePrimitiveBuffer.ReleaseAndGetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateBuffer(RC scene primitives) failed: 0x%08X\n", Result);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
    ShaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
    ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    ShaderResourceViewDesc.Buffer.FirstElement = 0;
    ShaderResourceViewDesc.Buffer.NumElements = MaxScenePrimitives;

    Result = Device->CreateShaderResourceView(
        ScenePrimitiveBuffer.Get(),
        &ShaderResourceViewDesc,
        ScenePrimitiveShaderResourceView.ReleaseAndGetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateShaderResourceView(RC scene primitives) failed: 0x%08X\n", Result);
        return false;
    }

    return true;
}

u32 RadianceCascadeRenderer::UploadScenePrimitives(ID3D11DeviceContext* DeviceContext, const Level& GameLevel)
{
    std::array<ScenePrimitiveGpu, MaxScenePrimitives> ScenePrimitives = {};
    u32 PrimitiveCount = 0;

    // NOTE(ljh): Collider는 빛을 내지 않는 AABB occluder로 사용.
    for (const AABB& Collider : GameLevel.GetColliders())
    {
        if (PrimitiveCount >= MaxScenePrimitives)
        {
            break;
        }

        ScenePrimitiveGpu& Primitive = ScenePrimitives[PrimitiveCount++];
        Primitive.Shape[0] = Collider.Min.X;
        Primitive.Shape[1] = Collider.Min.Y;
        Primitive.Shape[2] = Collider.Max.X;
        Primitive.Shape[3] = Collider.Max.Y;
        Primitive.LightData[3] = 0.0f;
    }

    // NOTE(ljh): Light2D는 원형 emitter로 사용.
    for (const Entity& CurrentEntity : GameLevel.GetEntities())
    {
        if (!CurrentEntity.LightComponent || PrimitiveCount >= MaxScenePrimitives)
        {
            continue;
        }

        const Light2D& CurrentLight = *CurrentEntity.LightComponent;
        float CenterX = CurrentEntity.Transform.X;
        float CenterY = CurrentEntity.Transform.Y;

        if (CurrentEntity.SpriteComponent)
        {
            const Sprite& CurrentSprite = *CurrentEntity.SpriteComponent;
            CenterX += CurrentSprite.Width * CurrentEntity.Transform.ScaleX * 0.5f;
            CenterY += CurrentSprite.Height * CurrentEntity.Transform.ScaleY * 0.5f;
        }

        ScenePrimitiveGpu& Primitive = ScenePrimitives[PrimitiveCount++];
        Primitive.Shape[0] = CenterX;
        Primitive.Shape[1] = CenterY;
        Primitive.Shape[2] = (std::max)(CurrentLight.Radius, 1.0f);
        Primitive.LightData[0] = CurrentLight.R * CurrentLight.Intensity;
        Primitive.LightData[1] = CurrentLight.G * CurrentLight.Intensity;
        Primitive.LightData[2] = CurrentLight.B * CurrentLight.Intensity;
        Primitive.LightData[3] = 1.0f;
    }

    if (PrimitiveCount == 0)
    {
        return 0;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource = {};
    const HRESULT Result = DeviceContext->Map(
        ScenePrimitiveBuffer.Get(),
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &MappedResource);

    if (FAILED(Result))
    {
        std::printf("Map(RC scene primitives) failed: 0x%08X\n", Result);
        return 0;
    }

    std::copy_n(ScenePrimitives.data(), PrimitiveCount, static_cast<ScenePrimitiveGpu*>(MappedResource.pData));
    DeviceContext->Unmap(ScenePrimitiveBuffer.Get(), 0);

    return PrimitiveCount;
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

}
