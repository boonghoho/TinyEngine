#pragma once

#include "../Core/types.h"

namespace tiny
{

struct Camera2D;
struct CpuFrameTiming;
class RenderPipeline;

class DebugUi
{
public:
    void Draw(
        f32 FrameMilliseconds,
        f32 TargetFps,
        const CpuFrameTiming& CpuTiming,
        Camera2D& Camera,
        RenderPipeline& Pipeline);

private:
    void DrawMenuBar();
    void DrawPerformance(f32 FrameMilliseconds, f32 TargetFps, const CpuFrameTiming& CpuTiming, const RenderPipeline& Pipeline);
    void DrawLighting(RenderPipeline& Pipeline);
    void DrawCamera(Camera2D& Camera);

    bool bShowPerformance = false;
    bool bShowLighting = false;
    bool bShowCamera = false;
};

}
