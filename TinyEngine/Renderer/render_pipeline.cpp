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

    // NOTE(ljh): Profiler가 초기화되지 않아도 renderer 자체는 계속 사용할 수 있음
    (void)GpuProfiler.Initialize(Device);

    return true;
}

void RenderPipeline::Release()
{
    GpuProfiler.Release();
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

    GpuProfiler.BeginFrame(DeviceContext);

    // NOTE(ljh): 1. 조명의 영향을 받기 전 sprite/tile 원본 색상을 그린다.
    GpuProfiler.BeginPass(DeviceContext, GpuPass::Sprites);
    GraphicsDevice.BeginGpuEvent(L"RenderSprites");
    RenderSprites(GraphicsDevice, GameLevel, Camera);
    GraphicsDevice.EndGpuEvent();
    GpuProfiler.EndPass(DeviceContext, GpuPass::Sprites);

    // NOTE(ljh): 2. RC를 계산해 화면 크기의 조명 결과를 만든다.
    if (bUseRadianceCascades)
    {
        GpuProfiler.BeginPass(DeviceContext, GpuPass::Lighting);
        GraphicsDevice.BeginGpuEvent(L"RenderLighting");
        RenderLighting(GraphicsDevice, GameLevel);
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
    const Camera2D& Camera)
{
    const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    GraphicsDevice.SetRenderTarget(
        SpriteColorTexture.GetRenderTargetView(),
        SpriteColorTexture.GetWidth(),
        SpriteColorTexture.GetHeight());
    GraphicsDevice.ClearRenderTarget(SpriteColorTexture.GetRenderTargetView(), ClearColor);

    SpriteRenderer.Begin(GraphicsDevice.GetContext(), Camera);
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

void RenderPipeline::RenderLighting(D3D11Device& GraphicsDevice, const Level& GameLevel)
{
    RadianceCascades.GenerateLighting(GraphicsDevice, GameLevel);

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

}
