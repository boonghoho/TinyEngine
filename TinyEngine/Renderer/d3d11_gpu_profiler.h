#pragma once

#include "../Core/types.h"

#include <array>
#include <cstddef>
#include <d3d11.h>
#include <wrl/client.h>

namespace tiny
{

enum class GpuPass : u8
{
    Sprites,
    Lighting,
    Composite,
    Count,
};

struct GpuPassTiming
{
    f64 Milliseconds = 0.0;
    bool bIsValid = false;
};

// NOTE(ljh): CPU stall을 피하기 위해 query 결과를 즉시 기다리지 않고,
// 여러 frame slot을 돌려 쓰며 완료된 결과만 회수한다.
class D3D11GpuProfiler
{
public:
    D3D11GpuProfiler() = default;
    ~D3D11GpuProfiler() = default;

    D3D11GpuProfiler(const D3D11GpuProfiler&) = delete;
    D3D11GpuProfiler& operator=(const D3D11GpuProfiler&) = delete;

    bool Initialize(ID3D11Device* Device);
    void Release();

    void BeginFrame(ID3D11DeviceContext* DeviceContext);
    void BeginPass(ID3D11DeviceContext* DeviceContext, GpuPass Pass);
    void EndPass(ID3D11DeviceContext* DeviceContext, GpuPass Pass);
    void EndFrame(ID3D11DeviceContext* DeviceContext);

    const GpuPassTiming& GetTiming(GpuPass Pass) const;
    bool IsInitialized() const { return bIsInitialized; }

private:
    static constexpr std::size_t PassCount = static_cast<std::size_t>(GpuPass::Count);
    static constexpr std::size_t BufferedFrameCount = 4;

    struct PassQueries
    {
        Microsoft::WRL::ComPtr<ID3D11Query> Start;
        Microsoft::WRL::ComPtr<ID3D11Query> End;
        bool bIsActive = false;
        bool bWasRecorded = false;
    };

    struct FrameQueries
    {
        Microsoft::WRL::ComPtr<ID3D11Query> Disjoint;
        std::array<PassQueries, PassCount> Passes;
        bool bIsPending = false;
    };

    bool InitializeFrameQueries(ID3D11Device* Device, FrameQueries& Queries);
    void TryResolvePendingFrames(ID3D11DeviceContext* DeviceContext);
    bool TryResolveFrame(ID3D11DeviceContext* DeviceContext, FrameQueries& Queries);

    std::array<FrameQueries, BufferedFrameCount> Frames;
    FrameQueries* CurrentFrame = nullptr;
    std::size_t NextFrameIndex = 0;

    std::array<GpuPassTiming, PassCount> Timings;

    bool bIsInitialized = false;
};

}
