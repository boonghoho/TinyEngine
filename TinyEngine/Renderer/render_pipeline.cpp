#include "render_pipeline.h"

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

    if (!CompositePass.Initialize(Device, L"Shaders/composite_scene.hlsl"))
    {
        Release();
        return false;
    }

    return true;
}

void RenderPipeline::Release()
{
    CompositePass.Release();
    LightTexture.Release();
    SpriteColorTexture.Release();

    Width = 0;
    Height = 0;
}

void RenderPipeline::RenderFrame(D3D11Device& GraphicsDevice, const Level& GameLevel)
{
    RenderSprites(GraphicsDevice, GameLevel);
    RenderLighting(GraphicsDevice);
    CompositeScene(GraphicsDevice);
}

void RenderPipeline::RenderSprites(D3D11Device& GraphicsDevice, const Level& GameLevel)
{
    const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    GraphicsDevice.SetRenderTarget(
        SpriteColorTexture.GetRenderTargetView(),
        SpriteColorTexture.GetWidth(),
        SpriteColorTexture.GetHeight());
    GraphicsDevice.ClearRenderTarget(SpriteColorTexture.GetRenderTargetView(), ClearColor);

    SpriteRenderer.Begin(GraphicsDevice.GetContext());
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

void RenderPipeline::RenderLighting(D3D11Device& GraphicsDevice)
{
    // NOTE(ljh): 조명이 아직 없으므로 모든 pixel이 원래 색을 유지하도록 흰색으로 채운다.
    const float DefaultLight[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    GraphicsDevice.SetRenderTarget(
        LightTexture.GetRenderTargetView(),
        LightTexture.GetWidth(),
        LightTexture.GetHeight());
    GraphicsDevice.ClearRenderTarget(LightTexture.GetRenderTargetView(), DefaultLight);
}

void RenderPipeline::CompositeScene(D3D11Device& GraphicsDevice)
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
        0.0f,
        0.0f,
        0.0f,
        Inputs,
        2);
}

}
