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
#include "Renderer/render_pipeline.h"
#include "Renderer/texture2d.h"

#include <cstdio>

constexpr tiny::i32 WindowWidth = 1280;
constexpr tiny::i32 WindowHeight = 720;
constexpr tiny::f32 TargetFps = 144.0f;
constexpr tiny::f32 FrameDelayMilliseconds = 1000.0f / TargetFps;

int RunEngine(SDL_Window* Window, HWND WindowHandle)
{
    tiny::D3D11Device GraphicsDevice;
    if (!GraphicsDevice.Initialize(WindowHandle, WindowWidth, WindowHeight))
    {
        return 1;
    }

    tiny::RenderPipeline RenderPipeline;
    if (!RenderPipeline.Initialize(GraphicsDevice.GetDevice(), WindowWidth, WindowHeight))
    {
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
    if (!EditorGui.Initialize(Window, GraphicsDevice.GetDevice(), GraphicsDevice.GetContext()))
    {
        return 1;
    }

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

        }
        const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

        EditorGui.BeginFrame();

        if (GameInput.WasPressed(SDL_SCANCODE_ESCAPE))
        {
            bIsRunning = false;
        }

        if (!EditorGui.WantsKeyboard())
        {
            if (tiny::Entity* Player = GameLevel.GetEntityByID(CatID))
            {
                CatController.Update(
                    *Player,
                    GameInput,
                    DeltaSeconds,
                    GameLevel.GetColliders());
            }
        }

        ImGui::Begin("TinyEngine");
        ImGui::Text("FPS: %.1f / %.0f", CurrentFps, TargetFps);
        ImGui::Text("Frame: %.2f ms / %.2f ms", FrameTimeMilliseconds, FrameDelayMilliseconds);
        ImGui::End();

        GraphicsDevice.BeginFrame(ClearColor);
        RenderPipeline.RenderFrame(GraphicsDevice, GameLevel);

        EditorGui.EndFrame();
        GraphicsDevice.Present();

        const Uint64 FrameEndTime = SDL_GetTicks();
        const Uint64 FrameTime = FrameEndTime - FrameStartTime;

        if (FrameDelayMilliseconds > FrameTime)
        {
            SDL_Delay(static_cast<Uint32>(FrameDelayMilliseconds - FrameTime));
        }
    }

    return 0;
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

    const int Result = RunEngine(Window, WindowHandle);

    SDL_DestroyWindow(Window);
    SDL_Quit();

    return Result;
}
