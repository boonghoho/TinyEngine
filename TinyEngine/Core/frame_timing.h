#pragma once

#include "types.h"

namespace tiny
{

struct GameFrameTiming
{
    u64 FrameID = 0;
    u32 ThreadID = 0;
    f64 GameUpdateMilliseconds = 0.0;
    f64 FrameExtractionMilliseconds = 0.0;
    f64 WaitMilliseconds = 0.0;
    bool bIsValid = false;
};

struct CpuFrameTiming
{
    GameFrameTiming Game;
    u64 RenderFrameID = 0;
    u32 RenderThreadID = 0;
    f64 RenderSubmitMilliseconds = 0.0;
    f64 PresentMilliseconds = 0.0;
    bool bRepeatedGameFrame = false;
    bool bLightingEnabled = true;
    bool bIsValid = false;
};

}
