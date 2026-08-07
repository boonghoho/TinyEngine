#include "fullscreen_pass.h"

#include "shader.h"

#include <cstdio>

FullscreenPass::~FullscreenPass()
{
    Release();
}

bool FullscreenPass::Initialize(ID3D11Device* Device, const wchar_t* ShaderPath)
{
    Release();

    if (!Device || !ShaderPath)
    {
        return false;
    }

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
        Release();
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
        Release();
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
        &PointClampSamplerState
    );

    if (FAILED(Result))
    {
        std::printf("CreateSamplerState(PointClampSamplerState) failed: 0x%08X\n", Result);
        Release();
        return false;
    }

    return true;
}

void FullscreenPass::Release()
{
    if (PointClampSamplerState)
    {
        PointClampSamplerState->Release();
        PointClampSamplerState = nullptr;
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
    ID3D11ShaderResourceView* ShaderResourceView
)
{
    Render(
        DeviceContext,
        Width,
        Height,
        MouseX,
        MouseY,
        TimeSeconds,
        &ShaderResourceView,
        1
    );
}

void FullscreenPass::Render(
    ID3D11DeviceContext* DeviceContext,
    float Width,
    float Height,
    float MouseX,
    float MouseY,
    float TimeSeconds,
    ID3D11ShaderResourceView* const* ShaderResourceViews,
    unsigned int ShaderResourceViewCount,
    float Param0,
    float Param1,
    float Param2,
    float Param3
)
{
    FrameConstants Constants = {};
    Constants.Resolution[0] = Width;
    Constants.Resolution[1] = Height;
    Constants.Mouse[0] = MouseX;
    Constants.Mouse[1] = MouseY;
    Constants.Time = TimeSeconds;
    Constants.Param0 = Param0;
    Constants.Param1 = Param1;
    Constants.Param2 = Param2;
    Constants.Param3 = Param3;

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
    
    // 이전 pass가 남긴 alpha blend state를 이어받지 않는다.
    // nullptr은 blending이 꺼진 D3D11 기본 blend state다.
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    DeviceContext->PSSetConstantBuffers(0, 1, &FrameConstantBuffer);
    DeviceContext->PSSetShaderResources(0, ShaderResourceViewCount, ShaderResourceViews);
    DeviceContext->PSSetSamplers(0, 1, &PointClampSamplerState);

    DeviceContext->Draw(3, 0);

    ID3D11ShaderResourceView* NullShaderResourceViews[8] = {};
    DeviceContext->PSSetShaderResources(0, 8, NullShaderResourceViews);
}
