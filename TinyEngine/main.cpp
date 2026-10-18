#define SDL_MAIN_HANDLED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Core/frame_timing.h"
#include "Core/log.h"
#include "Core/types.h"
#include "Debug/debug_ui.h"
#include "Game/player_controller.h"
#include "Input/input.h"
#include "Level/level.h"
#include "Memory/memory_arena.h"
#include "Renderer/camera2d.h"
#include "Renderer/d3d11_device.h"
#include "Renderer/render_pipeline.h"
#include "Renderer/texture2d.h"
#include "UI/imgui_layer.h"

#include <array>
#include <filesystem>
#include <string>

constexpr tiny::i32 WindowWidth = 1280;
constexpr tiny::i32 WindowHeight = 720;
constexpr tiny::f32 TargetFps = 144.0f;
constexpr tiny::f32 FrameDelayMilliseconds = 1000.0f / TargetFps;

namespace
{

tiny::f64 GetElapsedMilliseconds(Uint64 StartCounter, Uint64 EndCounter, Uint64 Frequency)
{
    return static_cast<tiny::f64>(EndCounter - StartCounter) * 1000.0 / static_cast<tiny::f64>(Frequency);
}

// NOTE(ljh): RC demo를 위해 임의로 만든 Bouncing Light 설정 구조체와 관련 함수들
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
    const char* BasePath = SDL_GetBasePath();
    if (!BasePath)
    {
        TE_LOG_ERROR("Core", "SDL_GetBasePath failed: %s\n", SDL_GetError());
        return 1;
    }

    const std::u8string BasePathUtf8(reinterpret_cast<const char8_t*>(BasePath));
    const std::filesystem::path ExecutableDirectory(BasePathUtf8);

    std::error_code WorkingDirectoryError;
    std::filesystem::current_path(ExecutableDirectory, WorkingDirectoryError);

    if (WorkingDirectoryError)
    {
        TE_LOG_ERROR("Core", "Failed to set executable directory: %s\n", WorkingDirectoryError.message().c_str());
        return 1;
    }

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

    tiny::Camera2D GameCamera;
    GameCamera.Position = {static_cast<tiny::f32>(WindowWidth) * 0.5f, static_cast<tiny::f32>(WindowHeight) * 0.5f};
    GameCamera.ViewportWidth = static_cast<tiny::f32>(WindowWidth);
    GameCamera.ViewportHeight = static_cast<tiny::f32>(WindowHeight);

    // NOTE(ljh): 현재는 내가 예전에 만든 Magicat 이미지를 임시로 사용한다.
    tiny::Texture2D MagicatTexture;

    if (!MagicatTexture.LoadFromFile(GraphicsDevice.GetDevice(), "Assets/magicat.png"))
    {
        return 1;
    }

    tiny::Level GameLevel;

    // NOTE(ljh): 현재 tiled 로 만든 임의의 맵을 불러온다.
    if (!GameLevel.Initialize(GraphicsDevice.GetDevice(), "Assets/Maps/dungeon_001.json"))
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

    tiny::ImGuiLayer GuiLayer;
    if (!GuiLayer.Initialize(Window, GraphicsDevice.GetDevice(), GraphicsDevice.GetContext()))
    {
        TE_LOG_ERROR("Core", "Failed to initialize ImGui");
        return 1;
    }

    TE_LOG_INFO("Core", "Lighting Demo ready (%dx%d)", WindowWidth, WindowHeight);

    tiny::DebugUi DebugUi;
    const SDL_WindowID GameWindowID = SDL_GetWindowID(Window);
    bool bIsRunning = true;
    const Uint64 PerformanceFrequency = SDL_GetPerformanceFrequency();
    Uint64 PreviousFrameCounter = SDL_GetPerformanceCounter();
    tiny::f32 FrameTimeMilliseconds = FrameDelayMilliseconds;
    tiny::CpuFrameTiming PreviousCpuTiming;

    tiny::Input GameInput;
    tiny::PlayerController CatController;

    while (bIsRunning)
    {
        tiny::CpuFrameTiming CurrentCpuTiming;
        const Uint64 FrameStartCounter = SDL_GetPerformanceCounter();
        const tiny::f32 DeltaSeconds =
            static_cast<tiny::f32>(FrameStartCounter - PreviousFrameCounter) /
            static_cast<tiny::f32>(PerformanceFrequency);
        PreviousFrameCounter = FrameStartCounter;

        FrameTimeMilliseconds = DeltaSeconds * 1000.0f;

        const Uint64 FrameStartTime = SDL_GetTicks();

        GameInput.BeginFrame();

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            GuiLayer.ProcessEvent(Event);
            if (SDL_GetWindowFromEvent(&Event) == Window)
            {
                GameInput.ProcessEvent(Event);
            }

            if (Event.type == SDL_EVENT_QUIT ||
                (Event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && Event.window.windowID == GameWindowID))
            {
                bIsRunning = false;
            }
        }
        if (!bIsRunning)
        {
            break;
        }
        const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

        GuiLayer.BeginFrame();

        const Uint64 GameUpdateStartCounter = SDL_GetPerformanceCounter();
        if (SDL_GetKeyboardFocus() == Window && !GuiLayer.WantsKeyboard())
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

        CurrentCpuTiming.GameUpdateMilliseconds = GetElapsedMilliseconds(
            GameUpdateStartCounter, SDL_GetPerformanceCounter(), PerformanceFrequency);

        DebugUi.Draw(FrameTimeMilliseconds, TargetFps, PreviousCpuTiming, GameCamera, RenderPipeline);

        const Uint64 RenderSubmitStartCounter = SDL_GetPerformanceCounter();
        GraphicsDevice.BeginFrame(ClearColor);
        RenderPipeline.RenderFrame(GraphicsDevice, GameLevel, GameCamera);

        GuiLayer.EndFrame();
        const Uint64 PresentStartCounter = SDL_GetPerformanceCounter();
        GraphicsDevice.Present();
        const Uint64 PresentEndCounter = SDL_GetPerformanceCounter();

        CurrentCpuTiming.RenderSubmitMilliseconds = GetElapsedMilliseconds(
            RenderSubmitStartCounter, PresentStartCounter, PerformanceFrequency);
        CurrentCpuTiming.PresentMilliseconds = GetElapsedMilliseconds(
            PresentStartCounter, PresentEndCounter, PerformanceFrequency);
        CurrentCpuTiming.bIsValid = true;
        PreviousCpuTiming = CurrentCpuTiming;

        const Uint64 FrameEndTime = SDL_GetTicks();
        const Uint64 FrameTime = FrameEndTime - FrameStartTime;

        if (FrameDelayMilliseconds > FrameTime)
        {
            SDL_Delay(static_cast<Uint32>(FrameDelayMilliseconds - FrameTime));
        }
    }

    return 0;
}

int RunApplication()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        TE_LOG_ERROR("Core", "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* Window = SDL_CreateWindow(
        "TinyEngine",
        WindowWidth,
        WindowHeight,
        0);

    if (Window == nullptr)
    {
        TE_LOG_ERROR("Core", "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_PropertiesID WindowProperties = SDL_GetWindowProperties(Window);
    HWND WindowHandle = static_cast<HWND>(
        SDL_GetPointerProperty(WindowProperties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));

    if (WindowHandle == nullptr)
    {
        TE_LOG_ERROR("Core", "Failed to get HWND from SDL window\n");
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return 1;
    }

    const int Result = RunEngine(Window, WindowHandle);

    SDL_DestroyWindow(Window);
    SDL_Quit();

    return Result;
}

int main(int Argc, char** Argv)
{
    (void)Argc;
    (void)Argv;

    SDL_SetMainReady();

    // SDL 초기화 오류도 기록할 수 있도록 로그 파일을 먼저 연다.
    const char* BasePath = SDL_GetBasePath();
    if (BasePath)
    {
        const std::u8string BasePathUtf8(reinterpret_cast<const char8_t*>(BasePath));
        const std::filesystem::path LogPath = std::filesystem::path(BasePathUtf8) / "TinyEngine.log";
        if (!tiny::InitializeLogging(LogPath))
        {
            TE_LOG_WARNING("Log", "Could not open TinyEngine.log; continuing with console/debugger output");
        }
    }
    else
    {
        TE_LOG_WARNING("Log", "Could not locate log directory: %s", SDL_GetError());
    }

    TE_LOG_INFO("Core", "TinyEngine starting");
    const int Result = RunApplication();
    TE_LOG_INFO("Core", "TinyEngine stopped (exit code: %d)", Result);

    tiny::ShutdownLogging();
    return Result;
}
