#include "texture2d.h"

// NOTE(ljh): stb_image의 구현은 정확히 하나의 .cpp에서만 생성한다.
#define STBI_ONLY_PNG
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include "../../ThirdParty/stb/stb_image.h"

#include <cstddef>
#include <cstdio>
#include <limits>

bool Texture2D::LoadFromFile(ID3D11Device* Device, const char* FilePath)
{
    Texture.Reset();
    ShaderResourceView.Reset();
    Width = 0;
    Height = 0;

    if (!Device || !FilePath)
    {
        return false;
    }

    int ImageWidth = 0;
    int ImageHeight = 0;
    int SourceChannelCount = 0;

    stbi_uc* Pixels = stbi_load(
        FilePath,
        &ImageWidth,
        &ImageHeight,   
        &SourceChannelCount,
        STBI_rgb_alpha);

    if (!Pixels || ImageWidth <= 0 || ImageHeight <= 0)
    {
        std::printf(
            "stbi_load(Texture2D) failed: %s\n", 
            stbi_failure_reason() ? stbi_failure_reason() : "unknown error");
        stbi_image_free(Pixels);
        
        return false;
    }

    const UINT TextureWidth = static_cast<UINT>(ImageWidth);
    const UINT TextureHeight = static_cast<UINT>(ImageHeight);
    const std::size_t RowPitch = static_cast<std::size_t>(TextureWidth) * 4;
    const std::size_t PixelByteCount = RowPitch * TextureHeight;

    if (RowPitch > (std::numeric_limits<UINT>::max)() ||
        PixelByteCount > (std::numeric_limits<UINT>::max)())
    {
        std::printf("Texture2D image is too large\n");
        stbi_image_free(Pixels);
        return false;
    }

    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = TextureWidth;
    TextureDesc.Height = TextureHeight;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA InitialData = {};
    InitialData.pSysMem = Pixels;
    InitialData.SysMemPitch = static_cast<UINT>(RowPitch);

    const HRESULT Result = Device->CreateTexture2D(
        &TextureDesc,
        &InitialData,
        Texture.GetAddressOf());

    stbi_image_free(Pixels);

    if (FAILED(Result))
    {
        std::printf("CreateTexture2D(Texture2D) failed: 0x%08X\n", Result);
        return false;
    }

    const HRESULT ShaderResourceViewResult = Device->CreateShaderResourceView(
        Texture.Get(),
        nullptr,
        ShaderResourceView.GetAddressOf());

    if (FAILED(ShaderResourceViewResult))
    {
        std::printf(
            "CreateShaderResourceView(Texture2D) failed: 0x%08X\n",
            ShaderResourceViewResult);
        Texture.Reset();
        return false;
    }

    Width = ImageWidth;
    Height = ImageHeight;
    return true;
}
