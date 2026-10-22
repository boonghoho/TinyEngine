#pragma once

#include "../Core/types.h"
#include "render_frame_data.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

namespace tiny
{

// NOTE(ljh): 화면 픽셀 좌표를 받아 텍스처가 입혀진 사각형 하나를 렌더링한다.
class SpriteRenderer
{
public:
    SpriteRenderer() = default;
    ~SpriteRenderer() = default;

    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    bool Initialize(ID3D11Device* Device, int SceneWidth, int SceneHeight);

    // NOTE(ljh): Camera constant buffer의 수명과 갱신은 RenderPipeline이 관리한다.
    void Begin(ID3D11DeviceContext* DeviceContext, ID3D11Buffer* CameraConstantBuffer);

    void End();

    void Draw(
        ID3D11ShaderResourceView* TextureSRV,
        float X,
        float Y,
        float Width,
        float Height,
        const UVRect& SourceUV = {},
        const ColorTint& Tint = {}
    );

private:
    static constexpr u32 MaxSpritesPerBatch = 2048;

    struct SpriteVertex
    {
        float Position[2];
        float UV[2];
        float Color[4];
    };

    int SceneWidth = 0;
    int SceneHeight = 0;

    std::vector<SpriteDrawCommand> DrawCommands;
    ID3D11DeviceContext* CurrentDeviceContext = nullptr;
    ID3D11Buffer* CurrentCameraConstantBuffer = nullptr;
    bool bIsDrawing = false;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> PointClampSamplerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> AlphaBlendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthDisabledState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> CullNoneRasterizerState;
};

}
