#include "d3d11_gpu_profiler.h"

#include <algorithm>
#include <cstdio>

namespace tiny
{

namespace
{

std::size_t GetPassIndex(GpuPass Pass)
{
    return static_cast<std::size_t>(Pass);
}

}

bool D3D11GpuProfiler::Initialize(ID3D11Device* Device)
{
    Release();

    if (!Device)
    {
        return false;
    }

    for (FrameQueries& Queries : Frames)
    {
        if (!InitializeFrameQueries(Device, Queries))
        {
            Release();
            return false;
        }
    }

    bIsInitialized = true;
    return true;
}

void D3D11GpuProfiler::Release()
{
    CurrentFrame = nullptr;
    NextFrameIndex = 0;

    for (FrameQueries& Queries : Frames)
    {
        Queries.Disjoint.Reset();
        Queries.bIsPending = false;

        for (PassQueries& Pass : Queries.Passes)
        {
            Pass.Start.Reset();
            Pass.End.Reset();
            Pass.bIsActive = false;
            Pass.bWasRecorded = false;
        }
    }

    Timings = {};
    bIsInitialized = false;
}

void D3D11GpuProfiler::BeginFrame(ID3D11DeviceContext* DeviceContext)
{
    if (!bIsInitialized || !DeviceContext || CurrentFrame)
    {
        return;
    }

    TryResolvePendingFrames(DeviceContext);

    for (std::size_t Offset = 0; Offset < BufferedFrameCount; ++Offset)
    {
        const std::size_t CandidateFrameIndex = (NextFrameIndex + Offset) % BufferedFrameCount;
        FrameQueries& CandidateFrame = Frames[CandidateFrameIndex];

        if (CandidateFrame.bIsPending)
        {
            continue;
        }

        for (PassQueries& Pass : CandidateFrame.Passes)
        {
            Pass.bIsActive = false;
            Pass.bWasRecorded = false;
        }

        DeviceContext->Begin(CandidateFrame.Disjoint.Get());
        CurrentFrame = &CandidateFrame;
        NextFrameIndex = (CandidateFrameIndex + 1) % BufferedFrameCount;
        return;
    }

    // NOTE(ljh): GPU가 query ring보다 늦으면 CPU가 기다리지 않고 이번 frame 측정을 생략한다.
}

void D3D11GpuProfiler::BeginPass(ID3D11DeviceContext* DeviceContext, GpuPass Pass)
{
    const std::size_t PassIndex = GetPassIndex(Pass);
    if (!CurrentFrame || !DeviceContext || PassIndex >= PassCount)
    {
        return;
    }

    PassQueries& Queries = CurrentFrame->Passes[PassIndex];
    if (Queries.bIsActive)
    {
        return;
    }

    // NOTE(ljh): Timestamp query는 구간을 여는 query가 아니라
    // End()를 호출한 시점의 GPU timestamp 하나를 기록한다.
    DeviceContext->End(Queries.Start.Get());
    Queries.bIsActive = true;
}

void D3D11GpuProfiler::EndPass(ID3D11DeviceContext* DeviceContext, GpuPass Pass)
{
    const std::size_t PassIndex = GetPassIndex(Pass);
    if (!CurrentFrame || !DeviceContext || PassIndex >= PassCount)
    {
        return;
    }

    PassQueries& Queries = CurrentFrame->Passes[PassIndex];
    if (!Queries.bIsActive)
    {
        return;
    }

    DeviceContext->End(Queries.End.Get());
    Queries.bIsActive = false;
    Queries.bWasRecorded = true;
}

void D3D11GpuProfiler::EndFrame(ID3D11DeviceContext* DeviceContext)
{
    if (!CurrentFrame || !DeviceContext)
    {
        return;
    }

    DeviceContext->End(CurrentFrame->Disjoint.Get());
    CurrentFrame->bIsPending = true;
    CurrentFrame = nullptr;
}

const GpuPassTiming& D3D11GpuProfiler::GetTiming(GpuPass Pass) const
{
    static const GpuPassTiming EmptyTiming;
    const std::size_t PassIndex = GetPassIndex(Pass);

    return PassIndex < PassCount ? Timings[PassIndex] : EmptyTiming;
}

bool D3D11GpuProfiler::InitializeFrameQueries(ID3D11Device* Device, FrameQueries& Queries)
{
    D3D11_QUERY_DESC QueryDesc = {};
    QueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    HRESULT Result = Device->CreateQuery(&QueryDesc, Queries.Disjoint.ReleaseAndGetAddressOf());
    if (FAILED(Result))
    {
        std::printf("CreateQuery(TimestampDisjoint) failed: 0x%08X\n", Result);
        return false;
    }

    QueryDesc.Query = D3D11_QUERY_TIMESTAMP;

    for (PassQueries& Pass : Queries.Passes)
    {
        Result = Device->CreateQuery(&QueryDesc, Pass.Start.ReleaseAndGetAddressOf());
        if (FAILED(Result))
        {
            std::printf("CreateQuery(Timestamp Start) failed: 0x%08X\n", Result);
            return false;
        }

        Result = Device->CreateQuery(&QueryDesc, Pass.End.ReleaseAndGetAddressOf());
        if (FAILED(Result))
        {
            std::printf("CreateQuery(Timestamp End) failed: 0x%08X\n", Result);
            return false;
        }
    }

    return true;
}

void D3D11GpuProfiler::TryResolvePendingFrames(ID3D11DeviceContext* DeviceContext)
{
    for (FrameQueries& Queries : Frames)
    {
        if (Queries.bIsPending)
        {
            TryResolveFrame(DeviceContext, Queries);
        }
    }
}

bool D3D11GpuProfiler::TryResolveFrame(ID3D11DeviceContext* DeviceContext, FrameQueries& Queries)
{
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData = {};

    // NOTE(ljh): GPU 작업을 강제로 진행시키지 않고,
    // 결과가 아직 준비되지 않았으면 다음 frame에 다시 확인한다.
    HRESULT Result = DeviceContext->GetData(
        Queries.Disjoint.Get(),
        &DisjointData,
        sizeof(DisjointData),
        D3D11_ASYNC_GETDATA_DONOTFLUSH);

    if (Result == S_FALSE)
    {
        return false;
    }

    if (FAILED(Result))
    {
        std::printf("GetData(TimestampDisjoint) failed: 0x%08X\n", Result);
        Queries.bIsPending = false;
        return true;
    }

    // NOTE(ljh): 측정 중 GPU timestamp clock이 불안정했으면
    // tick을 신뢰할 수 없으므로 이 frame의 결과를 버린다.
    if (DisjointData.Disjoint || DisjointData.Frequency == 0)
    {
        Queries.bIsPending = false;
        return true;
    }

    std::array<u64, PassCount> StartTimestamps = {};
    std::array<u64, PassCount> EndTimestamps = {};

    for (std::size_t PassIndex = 0; PassIndex < PassCount; ++PassIndex)
    {
        const PassQueries& Pass = Queries.Passes[PassIndex];
        if (!Pass.bWasRecorded)
        {
            continue;
        }

        Result = DeviceContext->GetData(
            Pass.Start.Get(),
            &StartTimestamps[PassIndex],
            sizeof(u64),
            D3D11_ASYNC_GETDATA_DONOTFLUSH);

        if (Result == S_FALSE)
        {
            return false;
        }

        if (FAILED(Result))
        {
            std::printf("GetData(Timestamp Start) failed: 0x%08X\n", Result);
            Queries.bIsPending = false;
            return true;
        }

        Result = DeviceContext->GetData(
            Pass.End.Get(),
            &EndTimestamps[PassIndex],
            sizeof(u64),
            D3D11_ASYNC_GETDATA_DONOTFLUSH);

        if (Result == S_FALSE)
        {
            return false;
        }

        if (FAILED(Result))
        {
            std::printf("GetData(Timestamp End) failed: 0x%08X\n", Result);
            Queries.bIsPending = false;
            return true;
        }
    }

    for (std::size_t PassIndex = 0; PassIndex < PassCount; ++PassIndex)
    {
        const PassQueries& Pass = Queries.Passes[PassIndex];

        if (!Pass.bWasRecorded || EndTimestamps[PassIndex] < StartTimestamps[PassIndex])
        {
            continue;
        }

        const f64 ElapsedTicks = static_cast<f64>(EndTimestamps[PassIndex] - StartTimestamps[PassIndex]);
        const f64 Milliseconds = ElapsedTicks / static_cast<f64>(DisjointData.Frequency) * 1000.0;
        Timings[PassIndex].Milliseconds = Milliseconds;
        Timings[PassIndex].bIsValid = true;
    }

    Queries.bIsPending = false;
    return true;
}

}
