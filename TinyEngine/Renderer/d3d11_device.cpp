#include "d3d11_device.h"

#include <cstdio>

#pragma comment(lib, "d3d11.lib")

D3D11Device::~D3D11Device()
{
    Release();
}

bool D3D11Device::Initialize(HWND WindowHandle, int InWidth, int InHeight)
{
    Release();

    if (!WindowHandle || InWidth <= 0 || InHeight <= 0)
    {
        return false;
    }

    Width = InWidth;
    Height = InHeight;

    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = 2;
    SwapChainDesc.BufferDesc.Width = Width;
    SwapChainDesc.BufferDesc.Height = Height;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.OutputWindow = WindowHandle;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.Windowed = TRUE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;

    UINT DeviceCreationFlags = 0;

#if defined(_DEBUG)
    DeviceCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT Result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        DeviceCreationFlags,
        &FeatureLevel,
        1,
        D3D11_SDK_VERSION,
        &SwapChainDesc,
        &SwapChain,
        &Device,
        nullptr,
        &DeviceContext
    );

    if (FAILED(Result))
    {
        std::printf("D3D11CreateDeviceAndSwapChain failed: 0x%08X\n", Result);
        Release();
        return false;
    }

    if (!CreateBackBufferRenderTarget())
    {
        Release();
        return false;
    }

    SetViewport(Width, Height);

    return true;
}

void D3D11Device::Release()
{
    if (BackBufferRenderTargetView)
    {
        BackBufferRenderTargetView->Release();
        BackBufferRenderTargetView = nullptr;
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

    Width = 0;
    Height = 0;
}

void D3D11Device::BeginFrame(const float ClearColor[4])
{
    SetBackBufferRenderTarget();
    ClearRenderTarget(BackBufferRenderTargetView, ClearColor);
}

void D3D11Device::SetBackBufferRenderTarget()
{
    SetRenderTarget(BackBufferRenderTargetView);
}

void D3D11Device::SetRenderTarget(ID3D11RenderTargetView* RenderTargetView)
{
    DeviceContext->OMSetRenderTargets(1, &RenderTargetView, nullptr);
    SetViewport(Width, Height);
}

void D3D11Device::SetRenderTarget(ID3D11RenderTargetView* RenderTargetView, int RenderTargetWidth, int RenderTargetHeight)
{
    DeviceContext->OMSetRenderTargets(1, &RenderTargetView, nullptr);
    SetViewport(RenderTargetWidth, RenderTargetHeight);
}

void D3D11Device::ClearRenderTarget(ID3D11RenderTargetView* RenderTargetView, const float ClearColor[4])
{
    DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
}

void D3D11Device::Present()
{
    SwapChain->Present(1, 0);
}

bool D3D11Device::CreateBackBufferRenderTarget()
{
    ID3D11Texture2D* BackBuffer = nullptr;
    HRESULT Result = SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));

    if (FAILED(Result))
    {
        std::printf("GetBuffer failed: 0x%08X\n", Result);
        return false;
    }

    Result = Device->CreateRenderTargetView(BackBuffer, nullptr, &BackBufferRenderTargetView);
    BackBuffer->Release();

    if (FAILED(Result))
    {
        std::printf("CreateRenderTargetView failed: 0x%08X\n", Result);
        return false;
    }

    return true;
}

void D3D11Device::SetViewport(int ViewportWidth, int ViewportHeight)
{
    D3D11_VIEWPORT Viewport = {};
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = static_cast<float>(ViewportWidth);
    Viewport.Height = static_cast<float>(ViewportHeight);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;

    DeviceContext->RSSetViewports(1, &Viewport);
}
