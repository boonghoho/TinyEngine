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

#include <array>
#include <cstdio>

constexpr tiny::i32 WindowWidth = 1280;
constexpr tiny::i32 WindowHeight = 720;
constexpr tiny::f32 TargetFps = 144.0f;
constexpr tiny::f32 FrameDelayMilliseconds = 1000.0f / TargetFps;

namespace
{

struct BouncingLightConfig
{
    tiny::Vec2 Position;
    tiny::Vec2 Velocity;
    tiny::Light2D Light;
};

constexpr std::array<BouncingLightConfig, 2> BouncingLightConfigs = {{
    {{160.0f, 160.0f}, {220.0f, 150.0f}, {0.0f, 1.0f, 0.0f, 25.0f, 12.0f}},
    {{1120.0f, 560.0f}, {-210.0f, -170.0f}, {0.0f, 0.0f, 1.0f, 25.0f, 12.0f}},
}};

void ReflectVelocity(tiny::Vec2& Velocity, const tiny::Vec2& SurfaceNormal)
{
    const tiny::f32 VelocityAlongNormal = Velocity.X * SurfaceNormal.X + Velocity.Y * SurfaceNormal.Y;

    Velocity.X -= 2.0f * VelocityAlongNormal * SurfaceNormal.X;
    Velocity.Y -= 2.0f * VelocityAlongNormal * SurfaceNormal.Y;
}

tiny::AABB MakeLightBounds(const tiny::Entity& LightEntity, tiny::f32 Radius)
{
    return {
        {LightEntity.Transform.X - Radius, LightEntity.Transform.Y - Radius},
        {LightEntity.Transform.X + Radius, LightEntity.Transform.Y + Radius},
    };
}

void UpdateBouncingLight(
    tiny::Entity& LightEntity,
    tiny::Vec2& Velocity,
    tiny::f32 DeltaSeconds,
    const std::vector<tiny::AABB>& Colliders)
{
    if (!LightEntity.LightComponent)
    {
        return;
    }

    const tiny::f32 SafeDeltaSeconds = DeltaSeconds < (1.0f / 30.0f) ? DeltaSeconds : (1.0f / 30.0f);
    const tiny::f32 Radius = LightEntity.LightComponent->Radius;

    LightEntity.Transform.X += Velocity.X * SafeDeltaSeconds;

    for (const tiny::AABB& Collider : Colliders)
    {
        if (!MakeLightBounds(LightEntity, Radius).Intersects(Collider))
        {
            continue;
        }

        if (Velocity.X > 0.0f)
        {
            LightEntity.Transform.X = Collider.Min.X - Radius;
            ReflectVelocity(Velocity, {-1.0f, 0.0f});
        }
        else if (Velocity.X < 0.0f)
        {
            LightEntity.Transform.X = Collider.Max.X + Radius;
            ReflectVelocity(Velocity, {1.0f, 0.0f});
        }

        break;
    }

    LightEntity.Transform.Y += Velocity.Y * SafeDeltaSeconds;

    for (const tiny::AABB& Collider : Colliders)
    {
        if (!MakeLightBounds(LightEntity, Radius).Intersects(Collider))
        {
            continue;
        }

        if (Velocity.Y > 0.0f)
        {
            LightEntity.Transform.Y = Collider.Min.Y - Radius;
            ReflectVelocity(Velocity, {0.0f, -1.0f});
        }
        else if (Velocity.Y < 0.0f)
        {
            LightEntity.Transform.Y = Collider.Max.Y + Radius;
            ReflectVelocity(Velocity, {0.0f, 1.0f});
        }

        break;
    }
}

}

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

    constexpr tiny::u32 BouncingLightCount = static_cast<tiny::u32>(BouncingLightConfigs.size());

    std::array<tiny::u32, BouncingLightCount> BouncingLightIDs = {};
    std::array<tiny::Vec2, BouncingLightCount> BouncingLightVelocities = {};

    for (tiny::u32 LightIndex = 0; LightIndex < BouncingLightCount; ++LightIndex)
    {
        const BouncingLightConfig& Config = BouncingLightConfigs[LightIndex];
        tiny::Entity& BouncingLight = GameLevel.CreateEntity();
        BouncingLightIDs[LightIndex] = BouncingLight.ID;
        BouncingLight.Transform.X = Config.Position.X;
        BouncingLight.Transform.Y = Config.Position.Y;
        BouncingLight.LightComponent = Config.Light;
        BouncingLightVelocities[LightIndex] = Config.Velocity;
    }

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

        for (tiny::u32 LightIndex = 0; LightIndex < BouncingLightCount; ++LightIndex)
        {
            if (tiny::Entity* CurrentLight = GameLevel.GetEntityByID(BouncingLightIDs[LightIndex]))
            {
                UpdateBouncingLight(
                    *CurrentLight,
                    BouncingLightVelocities[LightIndex],
                    DeltaSeconds,
                    GameLevel.GetColliders());
            }
        }

        ImGui::Begin("TinyEngine");
        ImGui::Text("FPS: %.1f / %.0f", CurrentFps, TargetFps);
        ImGui::Text("Frame: %.2f ms / %.2f ms", FrameTimeMilliseconds, FrameDelayMilliseconds);

        bool bEnableRadianceCascades = RenderPipeline.IsRadianceCascadesEnabled();
        if (ImGui::Checkbox("Radiance Cascades", &bEnableRadianceCascades))
        {
            RenderPipeline.SetLightingMode(
                bEnableRadianceCascades
                    ? tiny::LightingMode::RadianceCascades
                    : tiny::LightingMode::Unlit);
        }

        if (RenderPipeline.IsGpuProfilerAvailable())
        {
            const tiny::GpuPassTiming& SpriteTiming = RenderPipeline.GetGpuTiming(tiny::GpuPass::Sprites);
            const tiny::GpuPassTiming& LightingTiming = RenderPipeline.GetGpuTiming(tiny::GpuPass::Lighting);
            const tiny::GpuPassTiming& CompositeTiming = RenderPipeline.GetGpuTiming(tiny::GpuPass::Composite);

            ImGui::SeparatorText("GPU Scene Passes");

            if (SpriteTiming.bIsValid)
            {
                ImGui::Text(
                    "Sprites:   %.3f ms",
                    SpriteTiming.Milliseconds);
            }
            else
            {
                ImGui::TextUnformatted("Sprites:   collecting...");
            }

            if (!RenderPipeline.IsRadianceCascadesEnabled())
            {
                ImGui::TextUnformatted("Lighting:  N/A (disabled)");
            }
            else if (LightingTiming.bIsValid)
            {
                ImGui::Text(
                    "Lighting:  %.3f ms",
                    LightingTiming.Milliseconds);
            }
            else
            {
                ImGui::TextUnformatted("Lighting:  collecting...");
            }

            if (CompositeTiming.bIsValid)
            {
                ImGui::Text(
                    "Composite: %.3f ms",
                    CompositeTiming.Milliseconds);
            }
            else
            {
                ImGui::TextUnformatted("Composite: collecting...");
            }
        }
        else
        {
            ImGui::TextUnformatted("GPU profiler unavailable");
        }

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
