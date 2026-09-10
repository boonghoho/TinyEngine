#pragma once

#include <d3d11.h>
#include <wrl/client.h>

// NOTE(ljh): GPU가 렌더링 결과를 쓰고, 다른 Shader가 다시 읽을 수 있는 Texture를 관리한다. (ex. 렌더 파이프라인의 중간 결과)
namespace tiny
{

class RenderTexture
{
public:
    RenderTexture() = default;
    ~RenderTexture() = default;

    RenderTexture(const RenderTexture&) = delete;
    RenderTexture& operator=(const RenderTexture&) = delete;

    bool Initialize(ID3D11Device* Device, int InWidth, int InHeight);
    void Release();

    ID3D11RenderTargetView* GetRenderTargetView() const { return RenderTargetView.Get(); }
    ID3D11ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView.Get(); }
    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

private:
    int Width = 0;
    int Height = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
};

}
