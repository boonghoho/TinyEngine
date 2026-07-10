#pragma once

#include <cstdint>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <d3d11.h>

// NOTE(ljh): CPU에서 수정할 수 있는 Scene pixel을 저장하고 GPU Texture로 업로드한다.
class SceneTexture
{
public:
    SceneTexture() = default;
    ~SceneTexture();

    SceneTexture(const SceneTexture&) = delete;
    SceneTexture& operator=(const SceneTexture&) = delete;

    bool Initialize(ID3D11Device* Device, int Width, int Height);
    void Release();

    void Clear();
    void SetPixel(int X, int Y, std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A = 255);
    void DrawCircle(int CenterX, int CenterY, int Radius, std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A = 255);
    void DrawLine(int StartX, int StartY, int EndX, int EndY, int Radius, std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A = 255);
    void Upload(ID3D11DeviceContext* DeviceContext);

    ID3D11ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView; }

    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

private:
    int Width = 0;
    int Height = 0;

    std::vector<std::uint8_t> Pixels;

    ID3D11Texture2D* Texture = nullptr;
    ID3D11ShaderResourceView* ShaderResourceView = nullptr;
};
