#pragma once

struct ID3D11Device;
struct ID3D11DeviceContext;
struct SDL_Window;
union SDL_Event;

namespace tiny
{

// NOTE(ljh): Dear ImGui Context와 SDL3/DX11 Backend의 생명주기를 관리한다.
class ImGuiLayer
{
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    bool Initialize(
        SDL_Window* Window,
        ID3D11Device* Device,
        ID3D11DeviceContext* DeviceContext
    );

    void ProcessEvent(const SDL_Event& Event);
    void BeginFrame();
    void EndFrame();
    void Release();

    bool WantsMouse() const;
    bool WantsKeyboard() const;

private:
    bool bHasContext = false;
    bool bSdlBackendInitialized = false;
    bool bDx11BackendInitialized = false;
};

}
