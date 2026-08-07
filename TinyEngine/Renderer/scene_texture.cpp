#include "scene_texture.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace tiny
{

SceneTexture::~SceneTexture()
{
    Release();
}

bool SceneTexture::Initialize(ID3D11Device* Device, int InWidth, int InHeight)
{
    Release();

    if (!Device || InWidth <= 0 || InHeight <= 0)
    {
        return false;
    }

    Width = InWidth;
    Height = InHeight;

    Pixels.resize(Width * Height * 4);
    Clear();

    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = Width;
    TextureDesc.Height = Height;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA InitialData = {};
    InitialData.pSysMem = Pixels.data();
    InitialData.SysMemPitch = Width * 4;

    HRESULT Result = Device->CreateTexture2D(
        &TextureDesc,
        &InitialData,
        &Texture
    );

    if (FAILED(Result))
    {
        std::printf("CreateTexture2D failed: 0x%08X\n", Result);
        Release();
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
    ShaderResourceViewDesc.Format = TextureDesc.Format;
    ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    ShaderResourceViewDesc.Texture2D.MipLevels = 1;

    Result = Device->CreateShaderResourceView(
        Texture,
        &ShaderResourceViewDesc,
        &ShaderResourceView
    );

    if (FAILED(Result))
    {
        std::printf("CreateShaderResourceView failed: 0x%08X\n", Result);
        Release();
        return false;
    }

    return true;
}

void SceneTexture::Release()
{
    if (ShaderResourceView)
    {
        ShaderResourceView->Release();
        ShaderResourceView = nullptr;
    }

    if (Texture)
    {
        Texture->Release();
        Texture = nullptr;
    }

    Pixels.clear();
    Width = 0;
    Height = 0;
}

void SceneTexture::Clear()
{
    for (int Y = 0; Y < Height; ++Y)
    {
        for (int X = 0; X < Width; ++X)
        {
            const int Index = (Y * Width + X) * 4;

            Pixels[Index + 0] = 0;
            Pixels[Index + 1] = 0;
            Pixels[Index + 2] = 0;
            Pixels[Index + 3] = 255;
        }
    }
}

void SceneTexture::SetPixel(int X, int Y, std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A)
{
    if (X < 0 || X >= Width || Y < 0 || Y >= Height)
    {
        return;
    }

    const int Index = (Y * Width + X) * 4;

    Pixels[Index + 0] = R;
    Pixels[Index + 1] = G;
    Pixels[Index + 2] = B;
    Pixels[Index + 3] = A;
}

void SceneTexture::DrawCircle(int CenterX, int CenterY, int Radius, std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A)
{
    const int RadiusSquared = Radius * Radius;

    for (int Y = CenterY - Radius; Y <= CenterY + Radius; ++Y)
    {
        for (int X = CenterX - Radius; X <= CenterX + Radius; ++X)
        {
            const int DistanceX = X - CenterX;
            const int DistanceY = Y - CenterY;
            const int DistanceSquared = DistanceX * DistanceX + DistanceY * DistanceY;

            if (DistanceSquared <= RadiusSquared)
            {
                SetPixel(X, Y, R, G, B, A);
            }
        }
    }
}

void SceneTexture::DrawLine(int StartX, int StartY, int EndX, int EndY, int Radius, std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A)
{
    const int DeltaX = EndX - StartX;
    const int DeltaY = EndY - StartY;
    const int Steps = std::max(std::abs(DeltaX), std::abs(DeltaY));

    if (Steps == 0)
    {
        DrawCircle(StartX, StartY, Radius, R, G, B, A);
        return;
    }

    for (int Step = 0; Step <= Steps; ++Step)
    {
        const float T = static_cast<float>(Step) / static_cast<float>(Steps);
        const int X = static_cast<int>(StartX + DeltaX * T);
        const int Y = static_cast<int>(StartY + DeltaY * T);

        DrawCircle(X, Y, Radius, R, G, B, A);
    }
}

void SceneTexture::Upload(ID3D11DeviceContext* DeviceContext)
{
    DeviceContext->UpdateSubresource(
        Texture,
        0,
        nullptr,
        Pixels.data(),
        Width * 4,
        0
    );
}

}
