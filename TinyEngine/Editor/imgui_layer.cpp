#include "imgui_layer.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_sdl3.h>

namespace tiny
{

ImGuiLayer::~ImGuiLayer()
{
    Release();
}

bool ImGuiLayer::Initialize(
    SDL_Window* Window,
    ID3D11Device* Device,
    ID3D11DeviceContext* DeviceContext
)
{
    Release();

    if (!Window || !Device || !DeviceContext)
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    bHasContext = true;

    ImGuiIO& Io = ImGui::GetIO();
    Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForD3D(Window))
    {
        Release();
        return false;
    }
    bSdlBackendInitialized = true;

    if (!ImGui_ImplDX11_Init(Device, DeviceContext))
    {
        Release();
        return false;
    }
    bDx11BackendInitialized = true;

    return true;
}

void ImGuiLayer::ProcessEvent(const SDL_Event& Event)
{
    if (bSdlBackendInitialized)
    {
        ImGui_ImplSDL3_ProcessEvent(&Event);
    }
}

void ImGuiLayer::BeginFrame()
{
    if (!bHasContext || !bSdlBackendInitialized || !bDx11BackendInitialized)
    {
        return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(
        0,
        ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode
    );
}

void ImGuiLayer::EndFrame()
{
    if (!bHasContext || !bDx11BackendInitialized)
    {
        return;
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::Release()
{
    if (bDx11BackendInitialized)
    {
        ImGui_ImplDX11_Shutdown();
        bDx11BackendInitialized = false;
    }

    if (bSdlBackendInitialized)
    {
        ImGui_ImplSDL3_Shutdown();
        bSdlBackendInitialized = false;
    }

    if (bHasContext)
    {
        ImGui::DestroyContext();
        bHasContext = false;
    }
}

bool ImGuiLayer::WantsMouse() const
{
    return bHasContext && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::WantsKeyboard() const
{
    return bHasContext && ImGui::GetIO().WantCaptureKeyboard;
}

}
