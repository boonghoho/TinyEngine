#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class Texture2D
{
public:
    Texture2D() = default;
    ~Texture2D() = default;

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // NOTE(ljh): PNG 파일을 RGBA8로 읽어 GPU Texture와 SRV를 만든다.
    bool LoadFromFile(ID3D11Device* Device, const char* FilePath);

    ID3D11ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView.Get(); }
    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
    int Width = 0;
    int Height = 0;
};
