#include "fullscreen_pass.h"

#include "shader.h"

#include <cstdio>

FullscreenPass::~FullscreenPass()
{
    Release();
}

bool FullscreenPass::Initialize(ID3D11Device* Device, const wchar_t* ShaderPath)
{
    ID3DBlob* VertexShaderBlob = nullptr;
    ID3DBlob* PixelShaderBlob = nullptr;

    if (!CompileShaderFromFile(ShaderPath, "VSMain", "vs_5_0", &VertexShaderBlob))
    {
        return false;
    }

    if (!CompileShaderFromFile(ShaderPath, "PSMain", "ps_5_0", &PixelShaderBlob))
    {
        VertexShaderBlob->Release();
        return false;
    }

    HRESULT Result = Device->CreateVertexShader(
        VertexShaderBlob->GetBufferPointer(),
        VertexShaderBlob->GetBufferSize(),
        nullptr,
        &VertexShader
    );

    VertexShaderBlob->Release();

    if (FAILED(Result))
    {
        PixelShaderBlob->Release();
        std::printf("CreateVertexShader failed: 0x%08X\n", Result);
        return false;
    }

    Result = Device->CreatePixelShader(
        PixelShaderBlob->GetBufferPointer(),
        PixelShaderBlob->GetBufferSize(),
        nullptr,
        &PixelShader
    );

    PixelShaderBlob->Release();

    if (FAILED(Result))
    {
        std::printf("CreatePixelShader failed: 0x%08X\n", Result);
        return false;
    }

    D3D11_BUFFER_DESC ConstantBufferDesc = {};
    ConstantBufferDesc.ByteWidth = sizeof(FrameConstants);
    ConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    Result = Device->CreateBuffer(
        &ConstantBufferDesc,
        nullptr,
        &FrameConstantBuffer
    );

    if (FAILED(Result))
    {
        std::printf("CreateBuffer(FrameConstantBuffer) failed: 0x%08X\n", Result);
        return false;
    }

    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0.0f;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    Result = Device->CreateSamplerState(
        &SamplerDesc,
        &SceneSamplerState
    );

    if (FAILED(Result))
    {
        std::printf("CreateSamplerState(SceneSamplerState) failed: 0x%08X\n", Result);
        return false;
    }

    return true;
}

void FullscreenPass::Release()
{
    if (SceneSamplerState)
    {
        SceneSamplerState->Release();
        SceneSamplerState = nullptr;
    }

    if (FrameConstantBuffer)
    {
        FrameConstantBuffer->Release();
        FrameConstantBuffer = nullptr;
    }

    if (PixelShader)
    {
        PixelShader->Release();
        PixelShader = nullptr;
    }

    if (VertexShader)
    {
        VertexShader->Release();
        VertexShader = nullptr;
    }
}

void FullscreenPass::Render(
    ID3D11DeviceContext* DeviceContext,
    float Width,
    float Height,
    float MouseX,
    float MouseY,
    float TimeSeconds,
    ID3D11ShaderResourceView* SceneShaderResourceView
)
{
    FrameConstants Constants = {};
    Constants.Resolution[0] = Width;
    Constants.Resolution[1] = Height;
    Constants.Mouse[0] = MouseX;
    Constants.Mouse[1] = MouseY;
    Constants.Time = TimeSeconds;

    DeviceContext->UpdateSubresource(
        FrameConstantBuffer,
        0,
        nullptr,
        &Constants,
        0,
        0
    );

    DeviceContext->IASetInputLayout(nullptr);
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    DeviceContext->PSSetConstantBuffers(0, 1, &FrameConstantBuffer);
    DeviceContext->PSSetShaderResources(0, 1, &SceneShaderResourceView);
    DeviceContext->PSSetSamplers(0, 1, &SceneSamplerState);

    DeviceContext->Draw(3, 0);

    ID3D11ShaderResourceView* NullShaderResourceView = nullptr;
    DeviceContext->PSSetShaderResources(0, 1, &NullShaderResourceView);
}
