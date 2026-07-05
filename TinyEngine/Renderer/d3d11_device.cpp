#include "d3d11_device.h"

#include <cstdio>

#pragma comment(lib, "d3d11.lib")

D3D11Device::~D3D11Device()
{
    Release();
}

bool D3D11Device::Initialize(HWND WindowHandle, int InWidth, int InHeight)
{
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

    HRESULT Result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
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
        return false;
    }

    if (!CreateBackBufferRenderTarget())
    {
        return false;
    }

    D3D11_VIEWPORT Viewport = {};
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = static_cast<float>(Width);
    Viewport.Height = static_cast<float>(Height);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;

    DeviceContext->RSSetViewports(1, &Viewport);

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
}

void D3D11Device::BeginFrame(const float ClearColor[4])
{
    DeviceContext->OMSetRenderTargets(1, &BackBufferRenderTargetView, nullptr);
    DeviceContext->ClearRenderTargetView(BackBufferRenderTargetView, ClearColor);
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
