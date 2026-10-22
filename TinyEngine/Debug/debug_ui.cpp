#include "debug_ui.h"

#include "../Core/frame_timing.h"
#include "../Renderer/camera2d.h"
#include "../Renderer/render_pipeline.h"

#include <imgui.h>

namespace tiny
{

void DebugUi::Draw(
    f32 FrameMilliseconds,
    f32 TargetFps,
    const CpuFrameTiming& CpuTiming,
    Camera2D& Camera,
    RenderSettings& Settings,
    const RenderPipeline& Pipeline)
{
    UpdatePerformance(FrameMilliseconds, CpuTiming, Pipeline);
    DrawMenuBar();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (bShowLighting)
    {
        DrawLighting(Settings);
    }
    if (bShowCamera)
    {
        DrawCamera(Camera);
    }
    if (bShowPerformance)
    {
        DrawPerformance(TargetFps);
    }
}

void DebugUi::UpdatePerformance(f32 FrameMilliseconds, const CpuFrameTiming& CpuTiming, const RenderPipeline& Pipeline)
{
    PerformanceStats& Stats = PendingPerformance;
    if (FrameMilliseconds > 0.0f)
    {
        Stats.Frame.Add(FrameMilliseconds);
        PerformanceElapsedMilliseconds += FrameMilliseconds;
    }
    Stats.LatestCpu = CpuTiming;
    if (CpuTiming.bIsValid)
    {
        Stats.Submit.Add(CpuTiming.RenderSubmitMilliseconds);
        Stats.Present.Add(CpuTiming.PresentMilliseconds);
    }

    // 같은 게임 프레임을 다시 그려도 게임 측정값은 한 번만 평균에 넣는다.
    if (CpuTiming.Game.bIsValid && CpuTiming.Game.FrameID != LastSampledGameFrameID)
    {
        Stats.Update.Add(CpuTiming.Game.GameUpdateMilliseconds);
        Stats.Extract.Add(CpuTiming.Game.FrameExtractionMilliseconds);
        Stats.Wait.Add(CpuTiming.Game.WaitMilliseconds);
        LastSampledGameFrameID = CpuTiming.Game.FrameID;
    }

    Stats.bGpuAvailable = Pipeline.IsGpuProfilerAvailable();
    if (Stats.bGpuAvailable)
    {
        const auto SampleGpu = [&](GpuPass Pass, Average& Result)
        {
            const GpuPassTiming& Timing = Pipeline.GetGpuTiming(Pass);
            if (Timing.bIsValid)
            {
                Result.Add(Timing.Milliseconds);
            }
        };
        SampleGpu(GpuPass::Sprites, Stats.Sprites);
        if (CpuTiming.bLightingEnabled)
        {
            SampleGpu(GpuPass::Lighting, Stats.Lighting);
        }
        SampleGpu(GpuPass::Composite, Stats.Composite);
    }

    if (PerformanceElapsedMilliseconds >= PerformanceRefreshMilliseconds)
    {
        DisplayedPerformance = Stats;
        PendingPerformance = {};
        PerformanceElapsedMilliseconds = 0.0;
    }
}

void DebugUi::DrawMenuBar()
{
    ImGui::SetNextWindowBgAlpha(0.0f); // 메뉴 바 뒤에 불투명한 창 배경이 겹치지 않도록 한다.
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Profiler"))
        {
            ImGui::MenuItem("Performance", nullptr, &bShowPerformance);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Render"))
        {
            ImGui::MenuItem("Lighting", nullptr, &bShowLighting);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Camera"))
        {
            ImGui::MenuItem("Camera2D", nullptr, &bShowCamera);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void DebugUi::DrawPerformance(f32 TargetFps)
{
    const PerformanceStats& Stats = DisplayedPerformance;
    const CpuFrameTiming& CpuTiming = Stats.LatestCpu;
    const ImVec2 Origin = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos(ImVec2(Origin.x + 20.0f, Origin.y + 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(370.0f, 430.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Performance", &bShowPerformance))
    {
        ImGui::TextDisabled("Averages / refresh: %.2f s", PerformanceRefreshMilliseconds / 1000.0);
        ImGui::SetItemTooltip("Timing averages for each refresh interval. Frame IDs show the latest sample.\nOnly the display refresh is limited; game and rendering continue normally.");
        if (Stats.Frame.Count > 0)
        {
            const f64 FrameMilliseconds = Stats.Frame.Get();
            ImGui::Text("FPS: %.1f / %.0f", 1000.0 / FrameMilliseconds, TargetFps);
            ImGui::Text("Frame: %.2f ms / %.2f ms", FrameMilliseconds, 1000.0f / TargetFps);
        }
        else
        {
            ImGui::TextUnformatted("Collecting...");
        }

        ImGui::SeparatorText("Game CPU (average)");
        if (Stats.Update.Count > 0)
        {
            ImGui::Text("Thread: %u | Frame: %llu%s", CpuTiming.Game.ThreadID,
                static_cast<unsigned long long>(CpuTiming.Game.FrameID), CpuTiming.bRepeatedGameFrame ? " (reused)" : "");
            ImGui::SetItemTooltip("Latest sampled game frame. Reused means that render used the previous snapshot again.");
            ImGui::Text("Update:  %.3f ms", Stats.Update.Get());
            ImGui::SetItemTooltip("Player movement, collision and moving lights.");
            ImGui::Text("Extract: %.3f ms", Stats.Extract.Get());
            ImGui::SetItemTooltip("CPU time to copy camera, settings, sprites, lights and occluders into the render frame.");
            ImGui::Text("Wait:    %.3f ms", Stats.Wait.Get());
            ImGui::SetItemTooltip("Game thread wait for a free frame slot, including mutex and scheduling time.\nThe renderer keeps its current snapshot until a new one is ready.");
        }
        else
        {
            ImGui::TextUnformatted("No new game snapshot in this interval.");
        }

        ImGui::SeparatorText("Render CPU (average)");
        if (CpuTiming.bIsValid)
        {
            ImGui::Text("Thread: %u | Frame: %llu", CpuTiming.RenderThreadID,
                static_cast<unsigned long long>(CpuTiming.RenderFrameID));
            ImGui::Text("Submit:  %.3f ms", Stats.Submit.Get());
            ImGui::SetItemTooltip("CPU elapsed time for scene and ImGui rendering, including driver waits.\nIncludes detached window rendering and Present.\nExcludes the game window Present and FPS cap sleep.");
            ImGui::Text("Present: %.3f ms", Stats.Present.Get());
            ImGui::SetItemTooltip("CPU elapsed time inside the game window Present, including any wait.\nThis is not GPU execution time.");
        }
        else
        {
            ImGui::TextUnformatted("Collecting...");
        }

        ImGui::SeparatorText("GPU Scene Passes (sampled average)");
        ImGui::SetItemTooltip("Averages of the latest available GPU results sampled each render frame.\nGPU query results arrive with a delay.");
        if (Stats.bGpuAvailable)
        {
            if (Stats.Sprites.Count > 0)
            {
                ImGui::Text("Sprites:   %.3f ms", Stats.Sprites.Get());
            }
            else
            {
                ImGui::TextUnformatted("Sprites:   collecting...");
            }

            if (!CpuTiming.bLightingEnabled)
            {
                ImGui::TextUnformatted("Lighting:  N/A (disabled)");
            }
            else if (Stats.Lighting.Count > 0)
            {
                ImGui::Text("Lighting:  %.3f ms", Stats.Lighting.Get());
            }
            else
            {
                ImGui::TextUnformatted("Lighting:  collecting...");
            }

            if (Stats.Composite.Count > 0)
            {
                ImGui::Text("Composite: %.3f ms", Stats.Composite.Get());
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
    }
    ImGui::End();
}

void DebugUi::DrawLighting(RenderSettings& Settings)
{
    const ImVec2 Origin = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos(ImVec2(Origin.x + 380.0f, Origin.y + 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 120.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Lighting", &bShowLighting))
    {
        bool bEnableRadianceCascades = Settings.Lighting == LightingMode::RadianceCascades;
        if (ImGui::Checkbox("Radiance Cascades", &bEnableRadianceCascades))
        {
            Settings.Lighting = bEnableRadianceCascades ? LightingMode::RadianceCascades : LightingMode::Unlit;
        }
    }
    ImGui::End();
}

void DebugUi::DrawCamera(Camera2D& Camera)
{
    const ImVec2 Origin = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos(ImVec2(Origin.x + 380.0f, Origin.y + 160.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 150.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Camera2D", &bShowCamera))
    {
        ImGui::DragFloat("Camera X", &Camera.Position.X, 1.0f);
        ImGui::DragFloat("Camera Y", &Camera.Position.Y, 1.0f);
        ImGui::DragFloat("Zoom", &Camera.Zoom, 0.01f, 0.1f, 8.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    }
    ImGui::End();
}

}
