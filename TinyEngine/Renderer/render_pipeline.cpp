#include "render_pipeline.h"

#include "camera2d.h"
#include "d3d11_device.h"
#include "../Level/level.h"

namespace tiny
{

RenderPipeline::~RenderPipeline()
{
    Release();
}

bool RenderPipeline::Initialize(ID3D11Device* Device, int InWidth, int InHeight)
{
    Release();

    if (!Device || InWidth <= 0 || InHeight <= 0)
    {
        return false;
    }

    Width = InWidth;
    Height = InHeight;

    // 렌더 파이프라인에서 사용할 각 texture 와 renderer를 초기화한다.
    if (!SpriteColorTexture.Initialize(Device, Width, Height))
    {
        Release();
        return false;
    }

    if (!LightTexture.Initialize(Device, Width, Height))
    {
        Release();
        return false;
    }

    if (!SpriteRenderer.Initialize(Device, Width, Height))
    {
        Release();
        return false;
    }

    if (!RadianceCascades.Initialize(Device, Width, Height))
    {
        Release();
        return false;
    }

    if (!CompositePass.Initialize(Device, L"Shaders/composite_scene.hlsl"))
    {
        Release();
        return false;
    }

    D3D11_BUFFER_DESC CameraBufferDesc = {};
    CameraBufferDesc.ByteWidth = sizeof(CameraConstants);
    CameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    CameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    CameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT Result = Device->CreateBuffer(&CameraBufferDesc, nullptr, CameraConstantBuffer.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateBuffer(RenderPipeline CameraConstants) failed: 0x%08X\n", Result);
        Release();
        return false;
    }

    // NOTE(ljh): Profiler가 초기화되지 않아도 renderer 자체는 계속 사용할 수 있음
    (void)GpuProfiler.Initialize(Device);

    return true;
}

void RenderPipeline::Release()
{
    GpuProfiler.Release();
    CameraConstantBuffer.Reset();
    CompositePass.Release();
    RadianceCascades.Release();
    LightTexture.Release();
    SpriteColorTexture.Release();

    Width = 0;
    Height = 0;
}

void RenderPipeline::RenderFrame(
    D3D11Device& GraphicsDevice,
    const Level& GameLevel,
    const Camera2D& Camera)
{
    ID3D11DeviceContext* DeviceContext = GraphicsDevice.GetContext();
    const bool bUseRadianceCascades = IsRadianceCascadesEnabled();

    if (!UpdateCameraConstants(DeviceContext, Camera))
    {
        return;
    }

    GpuProfiler.BeginFrame(DeviceContext);

    // NOTE(ljh): 1. 조명의 영향을 받기 전 sprite/tile 원본 색상을 그린다.
    GpuProfiler.BeginPass(DeviceContext, GpuPass::Sprites);
    GraphicsDevice.BeginGpuEvent(L"RenderSprites");
    RenderSprites(GraphicsDevice, GameLevel, CameraConstantBuffer.Get());
    GraphicsDevice.EndGpuEvent();
    GpuProfiler.EndPass(DeviceContext, GpuPass::Sprites);

    // NOTE(ljh): 2. RC를 계산해 화면 크기의 조명 결과를 만든다.
    if (bUseRadianceCascades)
    {
        GpuProfiler.BeginPass(DeviceContext, GpuPass::Lighting);
        GraphicsDevice.BeginGpuEvent(L"RenderLighting");
        RenderLighting(GraphicsDevice, GameLevel, CameraConstantBuffer.Get());
        GraphicsDevice.EndGpuEvent();
        GpuProfiler.EndPass(DeviceContext, GpuPass::Lighting);
    }

    // NOTE(ljh): 3. 원본 색상과 조명을 곱하고 tone mapping하여 back buffer에 출력한다.
    GpuProfiler.BeginPass(DeviceContext, GpuPass::Composite);
    GraphicsDevice.BeginGpuEvent(L"CompositeScene");
    CompositeScene(GraphicsDevice, bUseRadianceCascades);
    GraphicsDevice.EndGpuEvent();
    GpuProfiler.EndPass(DeviceContext, GpuPass::Composite);

    GpuProfiler.EndFrame(DeviceContext);
}

void RenderPipeline::RenderSprites(
    D3D11Device& GraphicsDevice,
    const Level& GameLevel,
    ID3D11Buffer* CameraConstantBuffer)
{
    const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    GraphicsDevice.SetRenderTarget(
        SpriteColorTexture.GetRenderTargetView(),
        SpriteColorTexture.GetWidth(),
        SpriteColorTexture.GetHeight());
    GraphicsDevice.ClearRenderTarget(SpriteColorTexture.GetRenderTargetView(), ClearColor);

    SpriteRenderer.Begin(GraphicsDevice.GetContext(), CameraConstantBuffer);
    GameLevel.RenderTileMap(SpriteRenderer);

    for (const Entity& CurrentEntity : GameLevel.GetEntities())
    {
        if (!CurrentEntity.SpriteComponent)
        {
            continue;
        }

        const Sprite& CurrentSprite = *CurrentEntity.SpriteComponent;

        if (CurrentSprite.Texture == nullptr)
        {
            continue;
        }

        SpriteRenderer.Draw(
            CurrentSprite.Texture->GetShaderResourceView(),
            CurrentEntity.Transform.X,
            CurrentEntity.Transform.Y,
            CurrentSprite.Width * CurrentEntity.Transform.ScaleX,
            CurrentSprite.Height * CurrentEntity.Transform.ScaleY);
    }

    SpriteRenderer.End();
}

void RenderPipeline::RenderLighting(
    D3D11Device& GraphicsDevice,
    const Level& GameLevel,
    ID3D11Buffer* CameraConstantBuffer)
{
    RadianceCascades.GenerateLighting(GraphicsDevice, GameLevel, CameraConstantBuffer);

    const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    GraphicsDevice.SetRenderTarget(
        LightTexture.GetRenderTargetView(),
        LightTexture.GetWidth(),
        LightTexture.GetHeight());
    GraphicsDevice.ClearRenderTarget(LightTexture.GetRenderTargetView(), ClearColor);

    RadianceCascades.ResolveLighting(
        GraphicsDevice.GetContext(),
        Width,
        Height,
        0.1f,
        1.0f);
}

void RenderPipeline::CompositeScene(D3D11Device& GraphicsDevice, bool bUseLighting)
{
    GraphicsDevice.SetBackBufferRenderTarget();

    ID3D11ShaderResourceView* Inputs[] =
    {
        SpriteColorTexture.GetShaderResourceView(),
        LightTexture.GetShaderResourceView()
    };

    CompositePass.Render(
        GraphicsDevice.GetContext(),
        static_cast<float>(Width),
        static_cast<float>(Height),
        Inputs,
        2,
        1.5f,
        bUseLighting ? 1.0f : 0.0f);
}

bool RenderPipeline::UpdateCameraConstants(ID3D11DeviceContext* DeviceContext, const Camera2D& Camera)
{
    if (!DeviceContext || !CameraConstantBuffer ||
        Camera.Zoom <= 0.0f ||
        Camera.ViewportWidth <= 0.0f ||
        Camera.ViewportHeight <= 0.0f)
    {
        return false;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource = {};
    const HRESULT Result = DeviceContext->Map(
        CameraConstantBuffer.Get(),
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &MappedResource);

    if (FAILED(Result))
    {
        std::printf("Map(RenderPipeline CameraConstants) failed: 0x%08X\n", Result);
        return false;
    }

    CameraConstants* Constants = static_cast<CameraConstants*>(MappedResource.pData);
    *Constants = {};
    Constants->WorldToClip = Camera.BuildWorldToClip();
    Constants->ViewportSize[0] = Camera.ViewportWidth;
    Constants->ViewportSize[1] = Camera.ViewportHeight;
    Constants->Zoom = Camera.Zoom;

    DeviceContext->Unmap(CameraConstantBuffer.Get(), 0);
    return true;
}

}
