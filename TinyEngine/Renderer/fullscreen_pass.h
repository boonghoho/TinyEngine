#pragma once

#include <d3d11.h>
#include <wrl/client.h>

// NOTE(ljh): 하나 이상의 Texture를 입력받아 Fullscreen Shader Pass를 실행한다.
namespace tiny
{

class FullscreenPass
{
public:
    FullscreenPass() = default;
    ~FullscreenPass() = default;

    FullscreenPass(const FullscreenPass&) = delete;
    FullscreenPass& operator=(const FullscreenPass&) = delete;

    bool Initialize(ID3D11Device* Device, const wchar_t* ShaderPath);
    void Release();

    void Render(
        ID3D11DeviceContext* DeviceContext,
        float Width,
        float Height,
        ID3D11ShaderResourceView* ShaderResourceView
    );

    void Render(
        ID3D11DeviceContext* DeviceContext,
        float Width,
        float Height,
        ID3D11ShaderResourceView* const* ShaderResourceViews,
        unsigned int ShaderResourceViewCount,
        float Param0 = 0.0f,
        float Param1 = 0.0f
    );

private:
    struct FrameConstants
    {
        float Resolution[2];
        float Param0;
        float Param1;
    };

    Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> FrameConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> PointClampSamplerState;
};

}
