#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

// NOTE(ljh): D3D11 핵심 객체를 소유하고, Render Target 설정과 화면 출력을 관리한다.
namespace tiny
{

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
    void SetBackBufferRenderTarget();
    void SetRenderTarget(ID3D11RenderTargetView* RenderTargetView);
    void SetRenderTarget(ID3D11RenderTargetView* RenderTargetView, int RenderTargetWidth, int RenderTargetHeight);
    void ClearRenderTarget(ID3D11RenderTargetView* RenderTargetView, const float ClearColor[4]);
    void Present();

    ID3D11Device* GetDevice() const { return Device.Get(); }
    ID3D11DeviceContext* GetContext() const { return DeviceContext.Get(); }
    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

private:
    bool CreateBackBufferRenderTarget();
    void SetViewport(int ViewportWidth, int ViewportHeight);

    Microsoft::WRL::ComPtr<ID3D11Device> Device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> BackBufferRenderTargetView;

    int Width = 0;
    int Height = 0;
};

}
