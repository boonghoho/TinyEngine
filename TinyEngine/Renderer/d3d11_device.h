#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <d3d11.h>

class D3D11Device
{
public:
    D3D11Device() = default;
    ~D3D11Device();

    D3D11Device(const D3D11Device&) = delete;
    D3D11Device& operator=(const D3D11Device&) = delete;

    bool Initialize(HWND WindowHandle, int Width, int Height);
    void Release();

    void BeginFrame(const float ClearColor[4]);
    void Present();

    ID3D11Device* GetDevice() const { return Device; }
    ID3D11DeviceContext* GetContext() const { return DeviceContext; }
    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

private:
    bool CreateBackBufferRenderTarget();

    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
    ID3D11RenderTargetView* BackBufferRenderTargetView = nullptr;

    int Width = 0;
    int Height = 0;
};
