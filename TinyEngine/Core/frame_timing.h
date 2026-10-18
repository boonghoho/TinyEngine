#pragma once

#include "types.h"

namespace tiny
{

struct CpuFrameTiming
{
    f64 GameUpdateMilliseconds = 0.0;
    f64 RenderSubmitMilliseconds = 0.0;
    f64 PresentMilliseconds = 0.0;
    bool bIsValid = false;
};

}
