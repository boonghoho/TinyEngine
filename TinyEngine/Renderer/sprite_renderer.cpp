#include "sprite_renderer.h"

#include "shader.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

bool SpriteRenderer::Initialize(ID3D11Device* Device, int InSceneWidth, int InSceneHeight)
{
    if (!Device || InSceneWidth <= 0 || InSceneHeight <= 0)
    {
        return false;
    }

    SceneWidth = 0;
    SceneHeight = 0;
    VertexShader.Reset();
    PixelShader.Reset();
    InputLayout.Reset();
    VertexBuffer.Reset();
    IndexBuffer.Reset();
    PointClampSamplerState.Reset();
    AlphaBlendState.Reset();
    DepthDisabledState.Reset();
    CullNoneRasterizerState.Reset();

    Microsoft::WRL::ComPtr<ID3DBlob> VertexShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> PixelShaderBlob;

    if (!CompileShaderFromFile(
            L"Shaders/sprite.hlsl",
            "VSMain",
            "vs_5_0",
            VertexShaderBlob.GetAddressOf()) ||
        !CompileShaderFromFile(
            L"Shaders/sprite.hlsl",
            "PSMain",
            "ps_5_0",
            PixelShaderBlob.GetAddressOf()))
    {
        return false;
    }

    HRESULT Result = Device->CreateVertexShader(
        VertexShaderBlob->GetBufferPointer(),
        VertexShaderBlob->GetBufferSize(),
        nullptr,
        VertexShader.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateVertexShader(SpriteRenderer) failed: 0x%08X\n", Result);
        return false;
    }

    Result = Device->CreatePixelShader(
        PixelShaderBlob->GetBufferPointer(),
        PixelShaderBlob->GetBufferSize(),
        nullptr,
        PixelShader.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreatePixelShader(SpriteRenderer) failed: 0x%08X\n", Result);
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC InputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SpriteVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SpriteVertex, UV), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SpriteVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    Result = Device->CreateInputLayout(
        InputElements,
        static_cast<UINT>(sizeof(InputElements) / sizeof(InputElements[0])),
        VertexShaderBlob->GetBufferPointer(),
        VertexShaderBlob->GetBufferSize(),
        InputLayout.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateInputLayout(SpriteRenderer) failed: 0x%08X\n", Result);
        return false;
    }

    D3D11_BUFFER_DESC VertexBufferDesc = {};
    VertexBufferDesc.ByteWidth = sizeof(SpriteVertex) * 4;
    VertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Result = Device->CreateBuffer(
        &VertexBufferDesc,
        nullptr,
        VertexBuffer.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateBuffer(SpriteRenderer VertexBuffer) failed: 0x%08X\n", Result);
        return false;
    }

    const std::uint16_t Indices[] = { 0, 1, 2, 1, 3, 2 };

    D3D11_BUFFER_DESC IndexBufferDesc = {};
    IndexBufferDesc.ByteWidth = sizeof(Indices);
    IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA IndexData = {};
    IndexData.pSysMem = Indices;

    Result = Device->CreateBuffer(
        &IndexBufferDesc,
        &IndexData,
        IndexBuffer.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateBuffer(SpriteRenderer IndexBuffer) failed: 0x%08X\n", Result);
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
        PointClampSamplerState.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateSamplerState(SpriteRenderer) failed: 0x%08X\n", Result);
        return false;
    }

    D3D11_BLEND_DESC BlendDesc = {};
    BlendDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Result = Device->CreateBlendState(&BlendDesc, AlphaBlendState.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateBlendState(SpriteRenderer) failed: 0x%08X\n", Result);
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
    DepthStencilDesc.DepthEnable = FALSE;
    DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    DepthStencilDesc.StencilEnable = FALSE;

    Result = Device->CreateDepthStencilState(
        &DepthStencilDesc,
        DepthDisabledState.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateDepthStencilState(SpriteRenderer) failed: 0x%08X\n", Result);
        return false;
    }

    D3D11_RASTERIZER_DESC RasterizerDesc = {};
    RasterizerDesc.FillMode = D3D11_FILL_SOLID;
    RasterizerDesc.CullMode = D3D11_CULL_NONE;
    RasterizerDesc.DepthClipEnable = TRUE;

    Result = Device->CreateRasterizerState(
        &RasterizerDesc,
        CullNoneRasterizerState.GetAddressOf());

    if (FAILED(Result))
    {
        std::printf("CreateRasterizerState(SpriteRenderer) failed: 0x%08X\n", Result);
        return false;
    }

    SceneWidth = InSceneWidth;
    SceneHeight = InSceneHeight;
    return true;
}


void SpriteRenderer::RenderSprite(
    ID3D11DeviceContext* DeviceContext,
    ID3D11ShaderResourceView* TextureSRV,
    float X,
    float Y,
    float Width,
    float Height,
    const UVRect& SourceUV,
    const ColorTint& Tint)
{
    if (!DeviceContext || !TextureSRV ||
        !VertexShader || !PixelShader || !InputLayout || !VertexBuffer || !IndexBuffer ||
        !PointClampSamplerState || !AlphaBlendState || !DepthDisabledState || !CullNoneRasterizerState ||
        SceneWidth <= 0 || SceneHeight <= 0 ||
        Width <= 0.0f || Height <= 0.0f)
    {
        return;
    }

    const float Left = X * 2.0f / static_cast<float>(SceneWidth) - 1.0f;
    const float Right = (X + Width) * 2.0f / static_cast<float>(SceneWidth) - 1.0f;
    const float Top = 1.0f - Y * 2.0f / static_cast<float>(SceneHeight);
    const float Bottom = 1.0f - (Y + Height) * 2.0f / static_cast<float>(SceneHeight);

    const SpriteVertex Vertices[] =
    {
        { { Left,  Top },    { SourceUV.U0, SourceUV.V0 }, { Tint.R, Tint.G, Tint.B, Tint.A } },
        { { Right, Top },    { SourceUV.U1, SourceUV.V0 }, { Tint.R, Tint.G, Tint.B, Tint.A } },
        { { Left,  Bottom }, { SourceUV.U0, SourceUV.V1 }, { Tint.R, Tint.G, Tint.B, Tint.A } },
        { { Right, Bottom }, { SourceUV.U1, SourceUV.V1 }, { Tint.R, Tint.G, Tint.B, Tint.A } },
    };

    D3D11_MAPPED_SUBRESOURCE MappedVertexBuffer = {};
    const HRESULT Result = DeviceContext->Map(
        VertexBuffer.Get(),
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &MappedVertexBuffer);

    if (FAILED(Result))
    {
        std::printf("Map(SpriteRenderer VertexBuffer) failed: 0x%08X\n", Result);
        return;
    }

    std::memcpy(MappedVertexBuffer.pData, Vertices, sizeof(Vertices));
    DeviceContext->Unmap(VertexBuffer.Get(), 0);

    const UINT VertexStride = sizeof(SpriteVertex);
    const UINT VertexOffset = 0;
    ID3D11Buffer* VertexBufferPointer = VertexBuffer.Get();
    ID3D11SamplerState* SamplerState = PointClampSamplerState.Get();
    const float BlendFactor[4] = {};

    DeviceContext->OMSetBlendState(AlphaBlendState.Get(), BlendFactor, 0xFFFFFFFF);
    DeviceContext->OMSetDepthStencilState(DepthDisabledState.Get(), 0);
    DeviceContext->RSSetState(CullNoneRasterizerState.Get());
    DeviceContext->IASetInputLayout(InputLayout.Get());
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DeviceContext->IASetVertexBuffers(0, 1, &VertexBufferPointer, &VertexStride, &VertexOffset);
    DeviceContext->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    DeviceContext->VSSetShader(VertexShader.Get(), nullptr, 0);
    DeviceContext->PSSetShader(PixelShader.Get(), nullptr, 0);
    DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
    DeviceContext->PSSetSamplers(0, 1, &SamplerState);
    DeviceContext->DrawIndexed(6, 0, 0);

    ID3D11ShaderResourceView* NullTexture = nullptr;
    DeviceContext->PSSetShaderResources(0, 1, &NullTexture);
}
