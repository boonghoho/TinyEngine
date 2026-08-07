#include "render_texture.h"

#include <cstdio>

namespace tiny
{

RenderTexture::~RenderTexture()
{
    Release();
}

bool RenderTexture::Initialize(ID3D11Device* Device, int InWidth, int InHeight)
{
    Release();

    if (!Device || InWidth <= 0 || InHeight <= 0)
    {
        return false;
    }

    Width = InWidth;
    Height = InHeight;

    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = Width;
    TextureDesc.Height = Height;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT Result = Device->CreateTexture2D(
        &TextureDesc,
        nullptr,
        &Texture
    );

    if (FAILED(Result))
    {
        std::printf("CreateTexture2D(RenderTexture) failed: 0x%08X\n", Result);
        Release();
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc = {};
    RenderTargetViewDesc.Format = TextureDesc.Format;
    RenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    RenderTargetViewDesc.Texture2D.MipSlice = 0;

    Result = Device->CreateRenderTargetView(
        Texture,
        &RenderTargetViewDesc,
        &RenderTargetView
    );

    if (FAILED(Result))
    {
        std::printf("CreateRenderTargetView(RenderTexture) failed: 0x%08X\n", Result);
        Release();
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
    ShaderResourceViewDesc.Format = TextureDesc.Format;
    ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    ShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    ShaderResourceViewDesc.Texture2D.MipLevels = 1;

    Result = Device->CreateShaderResourceView(
        Texture,
        &ShaderResourceViewDesc,
        &ShaderResourceView
    );

    if (FAILED(Result))
    {
        std::printf("CreateShaderResourceView(RenderTexture) failed: 0x%08X\n", Result);
        Release();
        return false;
    }

    return true;
}

void RenderTexture::Release()
{
    if (ShaderResourceView)
    {
        ShaderResourceView->Release();
        ShaderResourceView = nullptr;
    }

    if (RenderTargetView)
    {
        RenderTargetView->Release();
        RenderTargetView = nullptr;
    }

    if (Texture)
    {
        Texture->Release();
        Texture = nullptr;
    }

    Width = 0;
    Height = 0;
}

ID3D11RenderTargetView* RenderTexture::GetRenderTargetView() const
{
    return RenderTargetView;
}

ID3D11ShaderResourceView* RenderTexture::GetShaderResourceView() const
{
    return ShaderResourceView;
}

}
