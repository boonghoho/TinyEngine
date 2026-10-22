#pragma once

#include "frame_timing.h"
#include "../Input/input.h"
#include "../Renderer/render_frame_data.h"

#include <array>
#include <condition_variable>
#include <mutex>

namespace tiny
{

struct GameFrameRequest
{
    Input GameInput;
    Camera2D Camera;
    RenderSettings Settings;
    bool bAcceptGameInput = false;
};

struct GameFrame
{
    RenderFrameData RenderData;
    GameFrameTiming Timing;
};

class FrameExchange
{
public:
    void SubmitRequest(const GameFrameRequest& Request);

    // 게임 스레드는 PublishFrame 이후 작성한 프레임에 접근하지 않는다.
    GameFrame* WaitForWritableFrame(GameFrameRequest& Request);
    void PublishFrame(GameFrame& Frame);

    // 메인 스레드는 다음 AcquireLatestFrame 호출 전까지 반환된 프레임을 읽는다.
    const GameFrame* AcquireLatestFrame();
    void Stop();

private:
    enum class SlotState
    {
        Free,
        Writing,
        Ready,
        Reading,
    };

    struct FrameSlot
    {
        GameFrame Frame;
        SlotState State = SlotState::Free;
    };

    FrameSlot* FindSlot(SlotState State);

    std::array<FrameSlot, 2> Slots;
    FrameSlot* ReadingSlot = nullptr;
    GameFrameRequest PendingRequest;
    std::mutex Mutex;
    std::condition_variable WritableCondition;
    bool bStopped = false;
};

}
