#pragma once

#include <d3d11.h>

// NOTE(ljh): GPU가 렌더링 결과를 쓰고, 다른 Shader가 다시 읽을 수 있는 Texture를 관리한다.
namespace tiny
{

class RenderTexture
{
public:
    RenderTexture() = default;
    ~RenderTexture();

    RenderTexture(const RenderTexture&) = delete;
    RenderTexture& operator=(const RenderTexture&) = delete;

    bool Initialize(ID3D11Device* Device, int InWidth, int InHeight);
    void Release();

    ID3D11RenderTargetView* GetRenderTargetView() const;
    ID3D11ShaderResourceView* GetShaderResourceView() const;
    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

private:
    int Width = 0;
    int Height = 0;

    ID3D11Texture2D* Texture = nullptr;
    ID3D11RenderTargetView* RenderTargetView = nullptr;
    ID3D11ShaderResourceView* ShaderResourceView = nullptr;
};

}
