#define SDL_MAIN_HANDLED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui.h>

#include "Editor/imgui_layer.h"
#include "Renderer/d3d11_device.h"
#include "Renderer/fullscreen_pass.h"
#include "Renderer/radiance_cascade_renderer.h"
#include "Renderer/scene_texture.h"
#include "Renderer/sprite_renderer.h"
#include "Memory/memory_arena.h"

#include <cstdint>
#include <cstdio>
#include "Renderer/texture2d.h"

constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;
constexpr float TargetFps = 144.0f;
constexpr float FrameDelayMilliseconds = 1000.0f / TargetFps;
constexpr int ViewModeScene = 0;
constexpr int ViewModeFirstCascade = 1;
constexpr int ViewModeMerge = ViewModeFirstCascade + RadianceCascadeRenderer::CascadeCount;
constexpr int ViewModeFinal = ViewModeMerge + 1;

void DrawDemoScene(SceneTexture& TargetSceneTexture)
{
    TargetSceneTexture.Clear();

    TargetSceneTexture.DrawLine(210, 345, 635, 375, 10, 70, 255, 90);
    TargetSceneTexture.DrawLine(400, 440, 690, 430, 9, 235, 255, 60);
    TargetSceneTexture.DrawLine(585, 565, 735, 525, 10, 255, 75, 55);

    TargetSceneTexture.DrawLine(260, 190, 260, 330, 13, 255, 70, 150);
    TargetSceneTexture.DrawLine(405, 180, 405, 315, 12, 75, 245, 255);
    TargetSceneTexture.DrawLine(530, 190, 530, 270, 11, 70, 255, 95);

    TargetSceneTexture.DrawLine(760, 135, 865, 415, 12, 210, 255, 60);
    TargetSceneTexture.DrawLine(1040, 115, 1065, 365, 15, 255, 82, 65);
    TargetSceneTexture.DrawLine(900, 100, 915, 165, 3, 255, 90, 65);

    TargetSceneTexture.DrawCircle(255, 515, 32, 255, 82, 65);
    TargetSceneTexture.DrawCircle(625, 640, 36, 255, 82, 65);
}

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
        0
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

    RadianceCascadeRenderer RadianceCascade;
    if (!RadianceCascade.Initialize(GraphicsDevice.GetDevice(), WindowWidth, WindowHeight))
    {
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    FullscreenPass DisplayPass;
    if (!DisplayPass.Initialize(GraphicsDevice.GetDevice(), L"Shaders/display_texture.hlsl"))
    {
        RadianceCascade.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    SceneTexture RcSceneTexture;
    if (!RcSceneTexture.Initialize(GraphicsDevice.GetDevice(), WindowWidth, WindowHeight))
    {
        DisplayPass.Release();
        RadianceCascade.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    // NOTE(ljh): Sprite Renderer test code
    SpriteRenderer SpritePreviewRenderer;
    if (!SpritePreviewRenderer.Initialize(GraphicsDevice.GetDevice(), WindowWidth, WindowHeight))
    {
        RcSceneTexture.Release();
        DisplayPass.Release();
        RadianceCascade.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    // NOTE(ljh): Texture2D test code
    Texture2D MagicatTexture;

    if (!MagicatTexture.LoadFromFile(GraphicsDevice.GetDevice(), "../Assets/magicat.png"))
    {
        return 1;
    }

    ImGuiLayer EditorGui;
    if (!EditorGui.Initialize(
            Window,
            GraphicsDevice.GetDevice(),
            GraphicsDevice.GetContext()))
    {
        RcSceneTexture.Release();
        DisplayPass.Release();
        RadianceCascade.Release();
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
    float FinalIndirectStrength = 1.0f;
    float FinalExposure = 1.2f;
    int ViewMode = ViewModeFinal;

    bool bIsRunning = true;

    while (bIsRunning)
    {
        const Uint64 FrameStartTime = SDL_GetTicks();

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            EditorGui.ProcessEvent(Event);

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
                !EditorGui.WantsKeyboard())
            {
                RcSceneTexture.Clear();
            }

            const bool bImGuiWantsMouse = EditorGui.WantsMouse();

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

        EditorGui.BeginFrame();

        ImGui::Begin("RC Paint");
        ImGui::ColorEdit3("Brush Color", BrushColor);
        ImGui::SliderInt("Brush Radius", &BrushRadius, 1, 64);
        ImGui::SliderFloat("Indirect", &FinalIndirectStrength, 0.0f, 8.0f);
        ImGui::SliderFloat("Exposure", &FinalExposure, 0.2f, 3.0f);
        ImGui::RadioButton("Scene", &ViewMode, ViewModeScene);
        for (int CascadeIndex = 0; CascadeIndex < RadianceCascadeRenderer::CascadeCount; ++CascadeIndex)
        {
            char CascadeLabel[8] = {};
            std::snprintf(CascadeLabel, sizeof(CascadeLabel), "C%d", CascadeIndex);

            ImGui::SameLine();
            ImGui::RadioButton(
                CascadeLabel,
                &ViewMode,
                ViewModeFirstCascade + CascadeIndex
            );
        }

        ImGui::SameLine();
        ImGui::RadioButton("Merge", &ViewMode, ViewModeMerge);
        ImGui::SameLine();
        ImGui::RadioButton("Final", &ViewMode, ViewModeFinal);
        if (ImGui::Button("Clear"))
        {
            RcSceneTexture.Clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Demo Scene"))
        {
            DrawDemoScene(RcSceneTexture);
            ViewMode = ViewModeFinal;
        }
        ImGui::End();

        // TODO(ljh): SceneTexture가 변경된 frame에만 GPU로 upload한다.
        RcSceneTexture.Upload(GraphicsDevice.GetContext());
        RadianceCascade.GenerateLighting(
            GraphicsDevice,
            RcSceneTexture.GetShaderResourceView()
        );

        GraphicsDevice.BeginFrame(ClearColor);

        if (ViewMode == ViewModeFinal)
        {
            RadianceCascade.DrawFinal(
                GraphicsDevice.GetContext(),
                RcSceneTexture.GetShaderResourceView(),
                GraphicsDevice.GetWidth(),
                GraphicsDevice.GetHeight(),
                FinalIndirectStrength,
                FinalExposure
            );
        }
        else
        {
            ID3D11ShaderResourceView* DisplayTexture = RcSceneTexture.GetShaderResourceView();

            if (ViewMode >= ViewModeFirstCascade &&
                ViewMode < ViewModeFirstCascade + RadianceCascadeRenderer::CascadeCount)
            {
                const int CascadeIndex = ViewMode - ViewModeFirstCascade;
                DisplayTexture = RadianceCascade.GetCascadeShaderResourceView(CascadeIndex);
            }
            else if (ViewMode == ViewModeMerge)
            {
                DisplayTexture = RadianceCascade.GetMergedShaderResourceView();
            }

            DisplayPass.Render(
                GraphicsDevice.GetContext(),
                static_cast<float>(GraphicsDevice.GetWidth()),
                static_cast<float>(GraphicsDevice.GetHeight()),
                MouseX,
                MouseY,
                TimeSeconds,
                DisplayTexture
            );
        }

        // TODO(ljh): 지금은 SpriteRenderer 와 Texture2D 구현을 확인하기 위해 임시로 사용.
        SpritePreviewRenderer.RenderSprite(
            GraphicsDevice.GetContext(),
            MagicatTexture.GetShaderResourceView(),
            24.0f, 24.0f,
            150.0f, 150.0f
        );

        EditorGui.EndFrame();
        GraphicsDevice.Present();

        const Uint64 FrameEndTime = SDL_GetTicks();
        const Uint64 FrameTime = FrameEndTime - FrameStartTime;

        if (FrameDelayMilliseconds > FrameTime)
        {
            SDL_Delay(static_cast<Uint32>(FrameDelayMilliseconds - FrameTime));
        }
    }

    EditorGui.Release();
    RcSceneTexture.Release();
    DisplayPass.Release();
    RadianceCascade.Release();
    GraphicsDevice.Release();
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
