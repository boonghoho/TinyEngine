#include "sprite_renderer.h"

#include "shader.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace tiny
{

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
    DrawCommands.clear();
    CurrentDeviceContext = nullptr;
    bIsDrawing = false;

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
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SpriteVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SpriteVertex, UV), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SpriteVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0},
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
    VertexBufferDesc.ByteWidth = sizeof(SpriteVertex) * 4 * MaxSpritesPerBatch;
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

    std::vector<std::uint16_t> Indices(MaxSpritesPerBatch * 6);

    for (unsigned int SpriteIndex = 0; SpriteIndex < MaxSpritesPerBatch; ++SpriteIndex)
    {
        const std::uint16_t VertexStart = static_cast<std::uint16_t>(SpriteIndex * 4);
        const unsigned int IndexStart = SpriteIndex * 6;

        Indices[IndexStart + 0] = VertexStart + 0;
        Indices[IndexStart + 1] = VertexStart + 1;
        Indices[IndexStart + 2] = VertexStart + 2;
        Indices[IndexStart + 3] = VertexStart + 1;
        Indices[IndexStart + 4] = VertexStart + 3;
        Indices[IndexStart + 5] = VertexStart + 2;
    }

    D3D11_BUFFER_DESC IndexBufferDesc = {};
    IndexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(std::uint16_t) * Indices.size());
    IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA IndexData = {};
    IndexData.pSysMem = Indices.data();

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
    DrawCommands.reserve(MaxSpritesPerBatch);
    return true;
}

void SpriteRenderer::Begin(ID3D11DeviceContext* DeviceContext)
{
    if (bIsDrawing || !DeviceContext)
    {
        return;
    }

    CurrentDeviceContext = DeviceContext;
    DrawCommands.clear();

    bIsDrawing = true;
}

void SpriteRenderer::End()
{
    if (!bIsDrawing || !CurrentDeviceContext)
    {
        return;
    }

    ID3D11DeviceContext* DeviceContext = CurrentDeviceContext;

    if (DrawCommands.empty())
    {
        CurrentDeviceContext = nullptr;
        bIsDrawing = false;
        return;
    }

    const UINT VertexStride = sizeof(SpriteVertex);
    const UINT VertexOffset = 0;
    ID3D11Buffer* VertexBufferPointer = VertexBuffer.Get();
    ID3D11SamplerState* SamplerState = PointClampSamplerState.Get();
    const float BlendFactor[4] = {};

    // NOTE(ljh): Sprite 가 공유하는 pipeline state 를 설정한다.
    DeviceContext->OMSetBlendState(AlphaBlendState.Get(), BlendFactor, 0xFFFFFFFF);
    DeviceContext->OMSetDepthStencilState(DepthDisabledState.Get(), 0);
    DeviceContext->RSSetState(CullNoneRasterizerState.Get());
    DeviceContext->IASetInputLayout(InputLayout.Get());
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DeviceContext->IASetVertexBuffers(0, 1, &VertexBufferPointer, &VertexStride, &VertexOffset);
    DeviceContext->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    DeviceContext->VSSetShader(VertexShader.Get(), nullptr, 0);
    DeviceContext->PSSetShader(PixelShader.Get(), nullptr, 0);
    DeviceContext->PSSetSamplers(0, 1, &SamplerState);

    std::size_t CommandStart = 0;

    while (CommandStart < DrawCommands.size())
    {
        // NOTE(ljh): 한 번에 그릴 수 있는 Sprite 의 최대 개수를 MaxSpritesPerBatch로 제한한다.
        const std::size_t CommandCount = std::min<std::size_t>(MaxSpritesPerBatch, DrawCommands.size() - CommandStart);

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
            break;
        }

        SpriteVertex* Vertices = static_cast<SpriteVertex*>(MappedVertexBuffer.pData);

        for (std::size_t CommandIndex = 0; CommandIndex < CommandCount; ++CommandIndex)
        {
            const SpriteDrawCommand& Command = DrawCommands[CommandStart + CommandIndex];

            const float Left = Command.X / static_cast<float>(SceneWidth) * 2.0f - 1.0f;
            const float Right = (Command.X + Command.Width) / static_cast<float>(SceneWidth) * 2.0f - 1.0f;
            const float Top = 1.0f - Command.Y / static_cast<float>(SceneHeight) * 2.0f;
            const float Bottom = 1.0f - (Command.Y + Command.Height) / static_cast<float>(SceneHeight) * 2.0f;
            const std::size_t VertexStart = CommandIndex * 4;

            Vertices[VertexStart + 0] = {
                {Left, Top},
                {Command.SourceUV.U0, Command.SourceUV.V0},
                {Command.Tint.R, Command.Tint.G, Command.Tint.B, Command.Tint.A}};
            Vertices[VertexStart + 1] = {
                {Right, Top},
                {Command.SourceUV.U1, Command.SourceUV.V0},
                {Command.Tint.R, Command.Tint.G, Command.Tint.B, Command.Tint.A}};
            Vertices[VertexStart + 2] = {
                {Left, Bottom},
                {Command.SourceUV.U0, Command.SourceUV.V1},
                {Command.Tint.R, Command.Tint.G, Command.Tint.B, Command.Tint.A}};
            Vertices[VertexStart + 3] = {
                {Right, Bottom},
                {Command.SourceUV.U1, Command.SourceUV.V1},
                {Command.Tint.R, Command.Tint.G, Command.Tint.B, Command.Tint.A}};
        }

        DeviceContext->Unmap(VertexBuffer.Get(), 0);

        std::size_t BatchStart = 0;

        // NOTE(ljh): DrawCommands 를 TextureSRV 별로 묶어서 한 번에 그린다. 추후 TextureSRV 별로 정렬해서 batch 효율을 높일 수 있음.
        while (BatchStart < CommandCount)
        {
            ID3D11ShaderResourceView* TextureSRV = DrawCommands[CommandStart + BatchStart].TextureSRV;
            std::size_t BatchEnd = BatchStart + 1;

            while (BatchEnd < CommandCount &&
                   DrawCommands[CommandStart + BatchEnd].TextureSRV == TextureSRV)
            {
                ++BatchEnd;
            }

            const UINT StartIndex = static_cast<UINT>(BatchStart * 6);
            const UINT IndexCount = static_cast<UINT>((BatchEnd - BatchStart) * 6);

            DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
            DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);

            BatchStart = BatchEnd;
        }

        CommandStart += CommandCount;
    }

    ID3D11ShaderResourceView* NullTexture = nullptr;
    DeviceContext->PSSetShaderResources(0, 1, &NullTexture);

    DrawCommands.clear();
    CurrentDeviceContext = nullptr;
    bIsDrawing = false;
}

void SpriteRenderer::Draw(
    ID3D11ShaderResourceView* TextureSRV,
    float X,
    float Y,
    float Width,
    float Height,
    const UVRect& SourceUV,
    const ColorTint& Tint)
{
    if (!bIsDrawing || !CurrentDeviceContext || !TextureSRV || Width <= 0.0f || Height <= 0.0f)
    {
        return;
    }

    SpriteDrawCommand Command;
    Command.TextureSRV = TextureSRV;

    Command.X = X;
    Command.Y = Y;
    Command.Width = Width;
    Command.Height = Height;

    Command.SourceUV = SourceUV;
    Command.Tint = Tint;

    DrawCommands.push_back(Command);
}

}
