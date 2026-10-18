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
    RenderPipeline& Pipeline)
{
    DrawMenuBar();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (bShowLighting)
    {
        DrawLighting(Pipeline);
    }
    if (bShowCamera)
    {
        DrawCamera(Camera);
    }
    if (bShowPerformance)
    {
        DrawPerformance(FrameMilliseconds, TargetFps, CpuTiming, Pipeline);
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

void DebugUi::DrawPerformance(
    f32 FrameMilliseconds,
    f32 TargetFps,
    const CpuFrameTiming& CpuTiming,
    const RenderPipeline& Pipeline)
{
    const ImVec2 Origin = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos(ImVec2(Origin.x + 20.0f, Origin.y + 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.0f, 310.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Performance", &bShowPerformance))
    {
        const f32 Fps = FrameMilliseconds > 0.0f ? 1000.0f / FrameMilliseconds : 0.0f;
        ImGui::Text("FPS: %.1f / %.0f", Fps, TargetFps);
        ImGui::Text("Frame: %.2f ms / %.2f ms", FrameMilliseconds, 1000.0f / TargetFps);

        ImGui::SeparatorText("CPU (last frame)");
        if (CpuTiming.bIsValid)
        {
            ImGui::Text("Update:  %.3f ms", CpuTiming.GameUpdateMilliseconds);
            ImGui::SetItemTooltip("Player movement, collision and moving lights.");
            ImGui::Text("Submit:  %.3f ms", CpuTiming.RenderSubmitMilliseconds);
            ImGui::SetItemTooltip("CPU elapsed time for scene and ImGui rendering, including driver waits.\nIncludes detached window rendering and Present.\nExcludes the game window Present and FPS cap sleep.");
            ImGui::Text("Present: %.3f ms", CpuTiming.PresentMilliseconds);
            ImGui::SetItemTooltip("CPU elapsed time inside the game window Present, including any wait.\nThis is not GPU execution time.");
        }
        else
        {
            ImGui::TextUnformatted("Collecting...");
        }

        ImGui::SeparatorText("GPU Scene Passes");
        if (Pipeline.IsGpuProfilerAvailable())
        {
            const GpuPassTiming& SpriteTiming = Pipeline.GetGpuTiming(GpuPass::Sprites);
            const GpuPassTiming& LightingTiming = Pipeline.GetGpuTiming(GpuPass::Lighting);
            const GpuPassTiming& CompositeTiming = Pipeline.GetGpuTiming(GpuPass::Composite);

            if (SpriteTiming.bIsValid)
            {
                ImGui::Text("Sprites:   %.3f ms", SpriteTiming.Milliseconds);
            }
            else
            {
                ImGui::TextUnformatted("Sprites:   collecting...");
            }

            if (!Pipeline.IsRadianceCascadesEnabled())
            {
                ImGui::TextUnformatted("Lighting:  N/A (disabled)");
            }
            else if (LightingTiming.bIsValid)
            {
                ImGui::Text("Lighting:  %.3f ms", LightingTiming.Milliseconds);
            }
            else
            {
                ImGui::TextUnformatted("Lighting:  collecting...");
            }

            if (CompositeTiming.bIsValid)
            {
                ImGui::Text("Composite: %.3f ms", CompositeTiming.Milliseconds);
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

void DebugUi::DrawLighting(RenderPipeline& Pipeline)
{
    const ImVec2 Origin = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos(ImVec2(Origin.x + 380.0f, Origin.y + 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 120.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Lighting", &bShowLighting))
    {
        bool bEnableRadianceCascades = Pipeline.IsRadianceCascadesEnabled();
        if (ImGui::Checkbox("Radiance Cascades", &bEnableRadianceCascades))
        {
            Pipeline.SetLightingMode(bEnableRadianceCascades ? LightingMode::RadianceCascades : LightingMode::Unlit);
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
