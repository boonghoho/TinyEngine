#pragma once

#include <d3d11.h>
#include <wrl/client.h>

// NOTE(ljh): 텍스처에서 사용할 정규화된 영역이다. 전체 텍스처는 { 0, 0, 1, 1 }이다.
namespace tiny
{

struct UVRect
{
    float U0 = 0.0f;
    float V0 = 0.0f;
    float U1 = 1.0f;
    float V1 = 1.0f;
};

struct ColorTint
{
    float R = 1.0f;
    float G = 1.0f;
    float B = 1.0f;
    float A = 1.0f;
};

// NOTE(ljh): 화면 픽셀 좌표를 받아 텍스처가 입혀진 사각형 하나를 렌더링한다.
class SpriteRenderer
{
public:
    SpriteRenderer() = default;
    ~SpriteRenderer() = default;

    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    bool Initialize(ID3D11Device* Device, int SceneWidth, int SceneHeight);

    void RenderSprite(
        ID3D11DeviceContext* DeviceContext,
        ID3D11ShaderResourceView* TextureSRV,
        float X,
        float Y,
        float Width,
        float Height,
        const UVRect& SourceUV = {},
        const ColorTint& Tint = {}
    );

private:
    struct SpriteVertex
    {
        float Position[2];
        float UV[2];
        float Color[4];
    };

    int SceneWidth = 0;
    int SceneHeight = 0;

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
