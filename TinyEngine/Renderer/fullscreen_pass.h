#pragma once

#include <d3d11.h>

class FullscreenPass
{
public:
    FullscreenPass() = default;
    ~FullscreenPass();

    FullscreenPass(const FullscreenPass&) = delete;
    FullscreenPass& operator=(const FullscreenPass&) = delete;

    bool Initialize(ID3D11Device* Device, const wchar_t* ShaderPath);
    void Release();

    void Render(
        ID3D11DeviceContext* DeviceContext,
        float Width,
        float Height,
        float MouseX,
        float MouseY,
        float TimeSeconds,
        ID3D11ShaderResourceView* SceneShaderResourceView
    );

private:
    struct FrameConstants
    {
        float Resolution[2];
        float Mouse[2];
        float Time;
        float Padding[3];
    };

    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11Buffer* FrameConstantBuffer = nullptr;
    ID3D11SamplerState* SceneSamplerState = nullptr;
};
