#pragma once

#include "../Core/frame_timing.h"

namespace tiny
{

struct Camera2D;
struct RenderSettings;
class RenderPipeline;

class DebugUi
{
public:
    void Draw(
        f32 FrameMilliseconds,
        f32 TargetFps,
        const CpuFrameTiming& CpuTiming,
        Camera2D& Camera,
        RenderSettings& Settings,
        const RenderPipeline& Pipeline);

private:
    struct Average
    {
        f64 Total = 0.0;
        u32 Count = 0;

        void Add(f64 Value)
        {
            Total += Value;
            ++Count;
        }

        f64 Get() const { return Count > 0 ? Total / Count : 0.0; }
    };

    struct PerformanceStats
    {
        Average Frame;
        Average Update;
        Average Extract;
        Average Wait;
        Average Submit;
        Average Present;
        Average Sprites;
        Average Lighting;
        Average Composite;
        CpuFrameTiming LatestCpu;
        bool bGpuAvailable = false;
    };

    static constexpr f64 PerformanceRefreshMilliseconds = 250.0;
    void UpdatePerformance(f32 FrameMilliseconds, const CpuFrameTiming& CpuTiming, const RenderPipeline& Pipeline);
    void DrawMenuBar();
    void DrawPerformance(f32 TargetFps);
    void DrawLighting(RenderSettings& Settings);
    void DrawCamera(Camera2D& Camera);

    bool bShowPerformance = false;
    bool bShowLighting = false;
    bool bShowCamera = false;
    PerformanceStats PendingPerformance;
    PerformanceStats DisplayedPerformance;
    f64 PerformanceElapsedMilliseconds = 0.0;
    u64 LastSampledGameFrameID = 0;
};

}
