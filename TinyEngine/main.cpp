#define SDL_MAIN_HANDLED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui.h>

#include "Core/types.h"
#include "Editor/imgui_layer.h"
#include "Game/player_controller.h"
#include "Input/input.h"
#include "Level/level.h"
#include "Memory/memory_arena.h"
#include "Renderer/d3d11_device.h"
#include "Renderer/fullscreen_pass.h"
#include "Renderer/radiance_cascade_renderer.h"
#include "Renderer/scene_texture.h"
#include "Renderer/sprite_renderer.h"
#include "Renderer/texture2d.h"

#include <cstdio>

constexpr tiny::i32 WindowWidth = 1280;
constexpr tiny::i32 WindowHeight = 720;
constexpr tiny::f32 TargetFps = 144.0f;
constexpr tiny::f32 FrameDelayMilliseconds = 1000.0f / TargetFps;
constexpr int ViewModeScene = 0;
constexpr int ViewModeFirstCascade = 1;
constexpr int ViewModeMerge = ViewModeFirstCascade + tiny::RadianceCascadeRenderer::CascadeCount;
constexpr int ViewModeFinal = ViewModeMerge + 1;

void DrawDemoScene(tiny::SceneTexture& TargetSceneTexture)
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
        0);

    if (Window == nullptr)
    {
        std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_PropertiesID WindowProperties = SDL_GetWindowProperties(Window);
    HWND WindowHandle = static_cast<HWND>(
        SDL_GetPointerProperty(WindowProperties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));

    if (WindowHandle == nullptr)
    {
        std::printf("Failed to get HWND from SDL window\n");
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    tiny::D3D11Device GraphicsDevice;
    if (!GraphicsDevice.Initialize(WindowHandle, WindowWidth, WindowHeight))
    {
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    tiny::RadianceCascadeRenderer RadianceCascade;
    if (!RadianceCascade.Initialize(GraphicsDevice.GetDevice(), WindowWidth, WindowHeight))
    {
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    tiny::FullscreenPass DisplayPass;
    if (!DisplayPass.Initialize(GraphicsDevice.GetDevice(), L"Shaders/display_texture.hlsl"))
    {
        RadianceCascade.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    tiny::SceneTexture RcSceneTexture;
    if (!RcSceneTexture.Initialize(GraphicsDevice.GetDevice(), WindowWidth, WindowHeight))
    {
        DisplayPass.Release();
        RadianceCascade.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    tiny::SpriteRenderer SpriteRenderer;
    if (!SpriteRenderer.Initialize(GraphicsDevice.GetDevice(), WindowWidth, WindowHeight))
    {
        RcSceneTexture.Release();
        DisplayPass.Release();
        RadianceCascade.Release();
        GraphicsDevice.Release();
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    // NOTE(ljh): 현재는 내가 예전에 만든 Magicat 이미지를 사용한다.
    tiny::Texture2D MagicatTexture;

    if (!MagicatTexture.LoadFromFile(GraphicsDevice.GetDevice(), "../Assets/magicat.png"))
    {
        return 1;
    }

    tiny::Level GameLevel;

    // NOTE(ljh): 현재 tiled 로 만든 임의의 맵을 불러온다.
    if (!GameLevel.Initialize(GraphicsDevice.GetDevice(), "../Assets/Maps/dungeon_001.json"))
    {
        return 1;
    }

    tiny::Entity& Cat = GameLevel.CreateEntity();
    const tiny::u32 CatID = Cat.ID;

    Cat.Transform.X = 500.0f;
    Cat.Transform.Y = 100.0f;
    Cat.Transform.ScaleX = 1.0f;
    Cat.Transform.ScaleY = 1.0f;

    Cat.SpriteComponent.emplace();

    Cat.SpriteComponent->Texture = &MagicatTexture;
    Cat.SpriteComponent->Width = 75.0f;
    Cat.SpriteComponent->Height = 75.0f;

    tiny::ImGuiLayer EditorGui;
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
    float BrushColor[3] = {1.0f, 0.70f, 0.16f};
    float FinalIndirectStrength = 1.0f;
    float FinalExposure = 1.2f;
    int ViewMode = ViewModeFinal;

    bool bIsRunning = true;
    const Uint64 PerformanceFrequency = SDL_GetPerformanceFrequency();
    Uint64 PreviousFrameCounter = SDL_GetPerformanceCounter();
    tiny::f32 FrameTimeMilliseconds = FrameDelayMilliseconds;
    tiny::f32 CurrentFps = TargetFps;

    tiny::Input GameInput;
    tiny::PlayerController CatController;

    while (bIsRunning)
    {
        const Uint64 FrameStartCounter = SDL_GetPerformanceCounter();
        const tiny::f32 DeltaSeconds =
            static_cast<tiny::f32>(FrameStartCounter - PreviousFrameCounter) /
            static_cast<tiny::f32>(PerformanceFrequency);
        PreviousFrameCounter = FrameStartCounter;

        FrameTimeMilliseconds = DeltaSeconds * 1000.0f;
        CurrentFps = DeltaSeconds > 0.0f ? 1.0f / DeltaSeconds : 0.0f;

        const Uint64 FrameStartTime = SDL_GetTicks();

        GameInput.BeginFrame();

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            EditorGui.ProcessEvent(Event);
            GameInput.ProcessEvent(Event);

            if (Event.type == SDL_EVENT_QUIT)
            {
                bIsRunning = false;
            }

            const bool bImGuiWantsMouse = EditorGui.WantsMouse();

            if (Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !bImGuiWantsMouse)
            {
                const int MouseX = static_cast<int>(Event.button.x);
                const int MouseY = static_cast<int>(Event.button.y);
                const tiny::u8 BrushColorR = static_cast<tiny::u8>(BrushColor[0] * 255.0f);
                const tiny::u8 BrushColorG = static_cast<tiny::u8>(BrushColor[1] * 255.0f);
                const tiny::u8 BrushColorB = static_cast<tiny::u8>(BrushColor[2] * 255.0f);

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
                        BrushColorB);
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
                const tiny::u8 BrushColorR = static_cast<tiny::u8>(BrushColor[0] * 255.0f);
                const tiny::u8 BrushColorG = static_cast<tiny::u8>(BrushColor[1] * 255.0f);
                const tiny::u8 BrushColorB = static_cast<tiny::u8>(BrushColor[2] * 255.0f);

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
                        BrushColorB);
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
                        0);
                }

                LastMouseX = MouseX;
                LastMouseY = MouseY;
            }
        }

        float MouseX = 0.0f;
        float MouseY = 0.0f;
        SDL_GetMouseState(&MouseX, &MouseY);

        const float TimeSeconds = SDL_GetTicks() / 1000.0f;
        const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

        EditorGui.BeginFrame();

        if (GameInput.WasPressed(SDL_SCANCODE_ESCAPE))
        {
            bIsRunning = false;
        }

        if (!EditorGui.WantsKeyboard())
        {
            if (GameInput.WasPressed(SDL_SCANCODE_C))
            {
                RcSceneTexture.Clear();
            }

            if (tiny::Entity* Player = GameLevel.GetEntityByID(CatID))
            {
                CatController.Update(
                    *Player,
                    GameInput,
                    DeltaSeconds,
                    GameLevel.GetColliders());
            }
        }

        ImGui::Begin("RC");
        ImGui::Text("FPS: %.1f / %.0f", CurrentFps, TargetFps);
        ImGui::Text("Frame: %.2f ms / %.2f ms", FrameTimeMilliseconds, FrameDelayMilliseconds);
        ImGui::Separator();
        ImGui::ColorEdit3("Brush Color", BrushColor);
        ImGui::SliderInt("Brush Radius", &BrushRadius, 1, 64);
        ImGui::SliderFloat("Indirect", &FinalIndirectStrength, 0.0f, 8.0f);
        ImGui::SliderFloat("Exposure", &FinalExposure, 0.2f, 3.0f);
        ImGui::RadioButton("Scene", &ViewMode, ViewModeScene);
        for (int CascadeIndex = 0; CascadeIndex < tiny::RadianceCascadeRenderer::CascadeCount; ++CascadeIndex)
        {
            char CascadeLabel[8] = {};
            std::snprintf(CascadeLabel, sizeof(CascadeLabel), "C%d", CascadeIndex);

            ImGui::SameLine();
            ImGui::RadioButton(
                CascadeLabel,
                &ViewMode,
                ViewModeFirstCascade + CascadeIndex);
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
            RcSceneTexture.GetShaderResourceView());

        GraphicsDevice.BeginFrame(ClearColor);

        if (ViewMode == ViewModeFinal)
        {
            RadianceCascade.DrawFinal(
                GraphicsDevice.GetContext(),
                RcSceneTexture.GetShaderResourceView(),
                GraphicsDevice.GetWidth(),
                GraphicsDevice.GetHeight(),
                FinalIndirectStrength,
                FinalExposure);
        }
        else
        {
            ID3D11ShaderResourceView* DisplayTexture = RcSceneTexture.GetShaderResourceView();

            if (ViewMode >= ViewModeFirstCascade &&
                ViewMode < ViewModeFirstCascade + tiny::RadianceCascadeRenderer::CascadeCount)
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
                DisplayTexture);
        }

        GameLevel.RenderTileMap(GraphicsDevice.GetContext(), SpriteRenderer);

        // NOTE(ljh): 현재 GameLevel의 Entity를 순회하며 SpriteComponent가 있는 경우 SpriteRenderer를 통해 화면에 렌더링한다.
        for (const tiny::Entity& CurrentEntity : GameLevel.GetEntities())
        {
            if (!CurrentEntity.SpriteComponent)
            {
                continue;
            }

            const tiny::Sprite& CurrentSprite = *CurrentEntity.SpriteComponent;

            if (CurrentSprite.Texture == nullptr)
            {
                continue;
            }

            SpriteRenderer.Render(
                GraphicsDevice.GetContext(),
                CurrentSprite.Texture->GetShaderResourceView(),
                CurrentEntity.Transform.X,
                CurrentEntity.Transform.Y,
                CurrentSprite.Width * CurrentEntity.Transform.ScaleX,
                CurrentSprite.Height * CurrentEntity.Transform.ScaleY);
        }

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
