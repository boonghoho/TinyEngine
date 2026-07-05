#define SDL_MAIN_HANDLED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_sdl3.h>

#include "Renderer/d3d11_device.h"
#include "Renderer/fullscreen_pass.h"
#include "Renderer/scene_texture.h"

#include <cstdint>
#include <cstdio>

constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;
constexpr float TargetFps = 144.0f;
constexpr float FrameDelayMilliseconds = 1000.0f / TargetFps;

int main(int Argc, char** Argv)
{
    (void)Argc;
    (void)Argv;

    SDL_SetMainReady();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* Window = SDL_CreateWindow(
        "TinyEngine",
        WindowWidth,
        WindowHeight,
        SDL_WINDOW_RESIZABLE
    );

    if (Window == nullptr)
    {
        std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_PropertiesID WindowProperties = SDL_GetWindowProperties(Window);
    HWND WindowHandle = static_cast<HWND>(
        SDL_GetPointerProperty(WindowProperties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr)
    );

    if (WindowHandle == nullptr)
    {
        std::printf("Failed to get HWND from SDL window\n");
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    D3D11Device GraphicsDevice;
    if (!GraphicsDevice.Initialize(WindowHandle, WindowWidth, WindowHeight))
    {
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    FullscreenPass Fullscreen;
    if (!Fullscreen.Initialize(GraphicsDevice.GetDevice(), L"Shaders/fullscreen.hlsl"))
    {
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    SceneTexture RcSceneTexture;
    if (!RcSceneTexture.Initialize(GraphicsDevice.GetDevice(), WindowWidth, WindowHeight))
    {
        Fullscreen.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForD3D(Window))
    {
        RcSceneTexture.Release();
        Fullscreen.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    if (!ImGui_ImplDX11_Init(GraphicsDevice.GetDevice(), GraphicsDevice.GetContext()))
    {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        RcSceneTexture.Release();
        Fullscreen.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    bool bLeftMouseDown = false;
    bool bEraseMouseDown = false;

    int LastMouseX = 0;
    int LastMouseY = 0;

    int BrushRadius = 8;
    float BrushColor[3] = { 1.0f, 0.70f, 0.16f };

    bool bIsRunning = true;

    while (bIsRunning)
    {
        const Uint64 FrameStartTime = SDL_GetTicks();

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            ImGui_ImplSDL3_ProcessEvent(&Event);

            if (Event.type == SDL_EVENT_QUIT)
            {
                bIsRunning = false;
            }

            if (Event.type == SDL_EVENT_KEY_DOWN && Event.key.key == SDLK_ESCAPE)
            {
                bIsRunning = false;
            }

            if (Event.type == SDL_EVENT_KEY_DOWN &&
                Event.key.scancode == SDL_SCANCODE_C &&
                !ImGui::GetIO().WantCaptureKeyboard)
            {
                RcSceneTexture.Clear();
            }

            const bool bImGuiWantsMouse = ImGui::GetIO().WantCaptureMouse;

            if (Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !bImGuiWantsMouse)
            {
                const int MouseX = static_cast<int>(Event.button.x);
                const int MouseY = static_cast<int>(Event.button.y);
                const std::uint8_t BrushColorR = static_cast<std::uint8_t>(BrushColor[0] * 255.0f);
                const std::uint8_t BrushColorG = static_cast<std::uint8_t>(BrushColor[1] * 255.0f);
                const std::uint8_t BrushColorB = static_cast<std::uint8_t>(BrushColor[2] * 255.0f);

                LastMouseX = MouseX;
                LastMouseY = MouseY;

                if (Event.button.button == SDL_BUTTON_LEFT)
                {
                    bLeftMouseDown = true;
                    RcSceneTexture.DrawCircle(
                        MouseX,
                        MouseY,
                        BrushRadius,
                        BrushColorR,
                        BrushColorG,
                        BrushColorB
                    );
                }

                if (Event.button.button == SDL_BUTTON_RIGHT)
                {
                    bEraseMouseDown = true;
                    RcSceneTexture.DrawCircle(MouseX, MouseY, BrushRadius, 0, 0, 0);
                }
            }

            if (Event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                if (Event.button.button == SDL_BUTTON_LEFT)
                {
                    bLeftMouseDown = false;
                }

                if (Event.button.button == SDL_BUTTON_RIGHT)
                {
                    bEraseMouseDown = false;
                }
            }

            if (Event.type == SDL_EVENT_MOUSE_MOTION && !bImGuiWantsMouse)
            {
                const int MouseX = static_cast<int>(Event.motion.x);
                const int MouseY = static_cast<int>(Event.motion.y);
                const std::uint8_t BrushColorR = static_cast<std::uint8_t>(BrushColor[0] * 255.0f);
                const std::uint8_t BrushColorG = static_cast<std::uint8_t>(BrushColor[1] * 255.0f);
                const std::uint8_t BrushColorB = static_cast<std::uint8_t>(BrushColor[2] * 255.0f);

                if (bLeftMouseDown)
                {
                    RcSceneTexture.DrawLine(
                        LastMouseX,
                        LastMouseY,
                        MouseX,
                        MouseY,
                        BrushRadius,
                        BrushColorR,
                        BrushColorG,
                        BrushColorB
                    );
                }

                if (bEraseMouseDown)
                {
                    RcSceneTexture.DrawLine(
                        LastMouseX,
                        LastMouseY,
                        MouseX,
                        MouseY,
                        BrushRadius,
                        0,
                        0,
                        0
                    );
                }

                LastMouseX = MouseX;
                LastMouseY = MouseY;
            }
        }

        float MouseX = 0.0f;
        float MouseY = 0.0f;
        SDL_GetMouseState(&MouseX, &MouseY);

        const float TimeSeconds = SDL_GetTicks() / 1000.0f;
        const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("RC Paint");
        ImGui::ColorEdit3("Brush Color", BrushColor);
        ImGui::SliderInt("Brush Radius", &BrushRadius, 1, 64);
        if (ImGui::Button("Clear"))
        {
            RcSceneTexture.Clear();
        }
        ImGui::End();

        ImGui::Render();

        RcSceneTexture.Upload(GraphicsDevice.GetContext());

        GraphicsDevice.BeginFrame(ClearColor);
        Fullscreen.Render(
            GraphicsDevice.GetContext(),
            static_cast<float>(GraphicsDevice.GetWidth()),
            static_cast<float>(GraphicsDevice.GetHeight()),
            MouseX,
            MouseY,
            TimeSeconds,
            RcSceneTexture.GetShaderResourceView()
        );
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        GraphicsDevice.Present();

        const Uint64 FrameEndTime = SDL_GetTicks();
        const Uint64 FrameTime = FrameEndTime - FrameStartTime;

        if (FrameDelayMilliseconds > FrameTime)
        {
            SDL_Delay(static_cast<Uint32>(FrameDelayMilliseconds - FrameTime));
        }
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    RcSceneTexture.Release();
    Fullscreen.Release();
    GraphicsDevice.Release();
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
