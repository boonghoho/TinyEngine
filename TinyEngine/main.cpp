#define SDL_MAIN_HANDLED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Core/frame_exchange.h"
#include "Core/log.h"
#include "Debug/debug_ui.h"
#include "Game/lighting_demo.h"
#include "Renderer/d3d11_device.h"
#include "Renderer/render_pipeline.h"
#include "UI/imgui_layer.h"

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <string>
#include <thread>

constexpr tiny::i32 WindowWidth = 1280;
constexpr tiny::i32 WindowHeight = 720;
constexpr tiny::f32 TargetFps = 300.0f;
constexpr tiny::f32 FrameDelayMilliseconds = 1000.0f / TargetFps;

namespace
{

using Clock = std::chrono::steady_clock;

tiny::f64 GetElapsedMilliseconds(Clock::time_point Start, Clock::time_point End)
{
    return std::chrono::duration<tiny::f64, std::milli>(End - Start).count();
}

void RunGameThread(tiny::LightingDemo& Demo, tiny::FrameExchange& Frames)
{
    const tiny::u32 ThreadID = GetCurrentThreadId();
    TE_LOG_INFO("Thread", "Game thread started (id: %u)", ThreadID);
    auto PreviousUpdate = Clock::now();
    tiny::u64 FrameID = 0;

    while (true)
    {
        tiny::GameFrameRequest Request;
        const auto WaitStart = Clock::now();
        tiny::GameFrame* Frame = Frames.WaitForWritableFrame(Request);
        if (!Frame)
        {
            break;
        }

        const auto UpdateStart = Clock::now();
        const tiny::f32 DeltaSeconds = std::chrono::duration<tiny::f32>(UpdateStart - PreviousUpdate).count();
        PreviousUpdate = UpdateStart;

        Demo.Update(Request.GameInput, Request.bAcceptGameInput, DeltaSeconds);
        const auto ExtractStart = Clock::now();
        Demo.BuildRenderFrame(Request.Camera, Request.Settings, Frame->RenderData);
        const auto ExtractEnd = Clock::now();

        Frame->Timing.FrameID = ++FrameID;
        Frame->Timing.ThreadID = ThreadID;
        Frame->Timing.WaitMilliseconds = GetElapsedMilliseconds(WaitStart, UpdateStart);
        Frame->Timing.GameUpdateMilliseconds = GetElapsedMilliseconds(UpdateStart, ExtractStart);
        Frame->Timing.FrameExtractionMilliseconds = GetElapsedMilliseconds(ExtractStart, ExtractEnd);
        Frame->Timing.bIsValid = true;
        Frames.PublishFrame(*Frame);
    }

    TE_LOG_INFO("Thread", "Game thread stopped (last frame: %llu)", static_cast<unsigned long long>(FrameID));
}

}

int RunEngine(SDL_Window* Window, HWND WindowHandle)
{
    const char* BasePath = SDL_GetBasePath();
    if (!BasePath)
    {
        TE_LOG_ERROR("Core", "SDL_GetBasePath failed: %s", SDL_GetError());
        return 1;
    }

    const std::u8string BasePathUtf8(reinterpret_cast<const char8_t*>(BasePath));
    const std::filesystem::path ExecutableDirectory(BasePathUtf8);
    std::error_code WorkingDirectoryError;
    std::filesystem::current_path(ExecutableDirectory, WorkingDirectoryError);
    if (WorkingDirectoryError)
    {
        TE_LOG_ERROR("Core", "Failed to set executable directory: %s", WorkingDirectoryError.message().c_str());
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

    // GPU 리소스는 시작 전에 만들고 게임 스레드 종료 후까지 유지한다.
    tiny::LightingDemo Demo;
    if (!Demo.Initialize(GraphicsDevice.GetDevice()))
    {
        return 1;
    }

    tiny::ImGuiLayer GuiLayer;
    if (!GuiLayer.Initialize(Window, GraphicsDevice.GetDevice(), GraphicsDevice.GetContext()))
    {
        TE_LOG_ERROR("Core", "Failed to initialize ImGui");
        return 1;
    }

    tiny::DebugUi DebugUi;
    tiny::GameFrameRequest Request;
    Request.Camera.Position = {static_cast<tiny::f32>(WindowWidth) * 0.5f, static_cast<tiny::f32>(WindowHeight) * 0.5f};
    Request.Camera.ViewportWidth = static_cast<tiny::f32>(WindowWidth);
    Request.Camera.ViewportHeight = static_cast<tiny::f32>(WindowHeight);

    tiny::FrameExchange Frames;
    Frames.SubmitRequest(Request);
    std::atomic<bool> bGameThreadFailed = false;
    std::jthread GameThread;
    try
    {
        GameThread = std::jthread([&](std::stop_token StopToken)
        {
            // 정상 종료와 스택 정리 모두 슬롯 대기를 깨운 뒤 join한다.
            std::stop_callback OnStop(StopToken, [&] { Frames.Stop(); });
            try
            {
                RunGameThread(Demo, Frames);
            }
            catch (const std::exception& Error)
            {
                TE_LOG_ERROR("Thread", "Game thread failed: %s", Error.what());
                bGameThreadFailed.store(true);
                Frames.Stop();
            }
            catch (...)
            {
                TE_LOG_ERROR("Thread", "Game thread failed with an unknown exception");
                bGameThreadFailed.store(true);
                Frames.Stop();
            }
        });
    }
    catch (const std::exception& Error)
    {
        TE_LOG_ERROR("Thread", "Could not start game thread: %s", Error.what());
        return 1;
    }

    const tiny::u32 RenderThreadID = GetCurrentThreadId();
    TE_LOG_INFO("Thread", "Main/render thread running (id: %u)", RenderThreadID);
    TE_LOG_INFO("Core", "Lighting Demo ready (%dx%d)", WindowWidth, WindowHeight);

    const SDL_WindowID GameWindowID = SDL_GetWindowID(Window);
    bool bIsRunning = true;
    auto PreviousFrameStart = Clock::now();
    tiny::CpuFrameTiming PreviousCpuTiming;
    tiny::u64 RenderFrameID = 0;

    while (bIsRunning && !bGameThreadFailed.load())
    {
        const auto FrameStart = Clock::now();
        const tiny::f32 FrameTimeMilliseconds = static_cast<tiny::f32>(GetElapsedMilliseconds(PreviousFrameStart, FrameStart));
        PreviousFrameStart = FrameStart;
        Request.GameInput.BeginFrame();

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            GuiLayer.ProcessEvent(Event);
            if (SDL_GetWindowFromEvent(&Event) == Window)
            {
                Request.GameInput.ProcessEvent(Event);
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

        GuiLayer.BeginFrame();
        DebugUi.Draw(FrameTimeMilliseconds, TargetFps, PreviousCpuTiming, Request.Camera, Request.Settings, RenderPipeline);
        Request.bAcceptGameInput = SDL_GetKeyboardFocus() == Window && !GuiLayer.WantsKeyboard();
        Frames.SubmitRequest(Request);

        // 새 프레임을 받을 때 이전 슬롯을 반환한다. 없으면 보관한 프레임을 다시 그린다.
        const tiny::GameFrame* Frame = Frames.AcquireLatestFrame();
        tiny::CpuFrameTiming CurrentCpuTiming;
        CurrentCpuTiming.RenderFrameID = ++RenderFrameID;
        CurrentCpuTiming.RenderThreadID = RenderThreadID;
        const auto RenderSubmitStart = Clock::now();
        const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        GraphicsDevice.BeginFrame(ClearColor);
        if (Frame)
        {
            CurrentCpuTiming.Game = Frame->Timing;
            CurrentCpuTiming.bRepeatedGameFrame = PreviousCpuTiming.Game.bIsValid &&
                Frame->Timing.FrameID == PreviousCpuTiming.Game.FrameID;
            CurrentCpuTiming.bLightingEnabled = Frame->RenderData.Settings.Lighting == tiny::LightingMode::RadianceCascades;
            RenderPipeline.RenderFrame(GraphicsDevice, Frame->RenderData);
        }

        GuiLayer.EndFrame();
        const auto PresentStart = Clock::now();
        GraphicsDevice.Present();
        const auto PresentEnd = Clock::now();
        CurrentCpuTiming.RenderSubmitMilliseconds = GetElapsedMilliseconds(RenderSubmitStart, PresentStart);
        CurrentCpuTiming.PresentMilliseconds = GetElapsedMilliseconds(PresentStart, PresentEnd);
        CurrentCpuTiming.bIsValid = true;
        PreviousCpuTiming = CurrentCpuTiming;

        const auto FrameMilliseconds = GetElapsedMilliseconds(FrameStart, Clock::now());
        if (FrameDelayMilliseconds > FrameMilliseconds)
        {
            SDL_Delay(static_cast<Uint32>(FrameDelayMilliseconds - FrameMilliseconds));
        }
    }

    GameThread.request_stop();
    GameThread.join();
    return bGameThreadFailed.load() ? 1 : 0;
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
