#include "frame_exchange.h"
#include "assert.h"

namespace tiny
{

void FrameExchange::SubmitRequest(const GameFrameRequest& Request)
{
    std::lock_guard<std::mutex> Lock(Mutex);
    if (bStopped)
    {
        return;
    }

    PendingRequest.Camera = Request.Camera;
    PendingRequest.Settings = Request.Settings;
    PendingRequest.bAcceptGameInput = Request.bAcceptGameInput;
    if (Request.bAcceptGameInput)
    {
        PendingRequest.GameInput.Accumulate(Request.GameInput);
    }
    else
    {
        PendingRequest.GameInput = Input{};
    }
}

GameFrame* FrameExchange::WaitForWritableFrame(GameFrameRequest& Request)
{
    std::unique_lock<std::mutex> Lock(Mutex);
    WritableCondition.wait(Lock, [this]
    {
        return bStopped || (FindSlot(SlotState::Ready) == nullptr && FindSlot(SlotState::Free) != nullptr);
    });
    if (bStopped)
    {
        return nullptr;
    }

    TE_ASSERT(FindSlot(SlotState::Writing) == nullptr);
    FrameSlot* Slot = FindSlot(SlotState::Free);
    Slot->State = SlotState::Writing;
    Request = PendingRequest;
    PendingRequest.GameInput.BeginFrame();
    return &Slot->Frame;
}

void FrameExchange::PublishFrame(GameFrame& Frame)
{
    std::lock_guard<std::mutex> Lock(Mutex);
    FrameSlot* Slot = FindSlot(SlotState::Writing);
    TE_ASSERT(Slot != nullptr && &Slot->Frame == &Frame);
    TE_ASSERT(FindSlot(SlotState::Ready) == nullptr);
    if (Slot == nullptr || &Slot->Frame != &Frame)
    {
        return;
    }

    Slot->State = bStopped ? SlotState::Free : SlotState::Ready;
}

const GameFrame* FrameExchange::AcquireLatestFrame()
{
    std::lock_guard<std::mutex> Lock(Mutex);
    FrameSlot* ReadySlot = bStopped ? nullptr : FindSlot(SlotState::Ready);
    if (ReadySlot != nullptr)
    {
        if (ReadingSlot != nullptr)
        {
            ReadingSlot->State = SlotState::Free;
        }
        ReadingSlot = ReadySlot;
        ReadingSlot->State = SlotState::Reading;
        WritableCondition.notify_one();
    }

    return ReadingSlot != nullptr ? &ReadingSlot->Frame : nullptr;
}

void FrameExchange::Stop()
{
    std::lock_guard<std::mutex> Lock(Mutex);
    bStopped = true;
    WritableCondition.notify_all();
}

FrameExchange::FrameSlot* FrameExchange::FindSlot(SlotState State)
{
    for (FrameSlot& Slot : Slots)
    {
        if (Slot.State == State)
        {
            return &Slot;
        }
    }
    return nullptr;
}

}
