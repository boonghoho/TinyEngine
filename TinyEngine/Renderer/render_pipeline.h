#pragma once

#include "fullscreen_pass.h"
#include "radiance_cascade_renderer.h"
#include "render_texture.h"
#include "sprite_renderer.h"

namespace tiny
{

class D3D11Device;
class Level;

// NOTE(ljh): 한 frame의 게임 장면을 어떤 순서로 그릴지 관리한다.
class RenderPipeline
{
public:
    RenderPipeline() = default;
    ~RenderPipeline();

    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    bool Initialize(ID3D11Device* Device, int Width, int Height);
    void Release();

    void RenderFrame(D3D11Device& GraphicsDevice, const Level& GameLevel);

private:
    void RenderSprites(D3D11Device& GraphicsDevice, const Level& GameLevel);
    void RenderLighting(D3D11Device& GraphicsDevice, const Level& GameLevel);
    void CompositeScene(D3D11Device& GraphicsDevice);

    int Width = 0;
    int Height = 0;

    SpriteRenderer SpriteRenderer;
    RenderTexture SpriteColorTexture;
    RenderTexture LightTexture;
    RadianceCascadeRenderer RadianceCascades;
    FullscreenPass CompositePass;
};

}
