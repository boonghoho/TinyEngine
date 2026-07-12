#include "memory_arena.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include <limits>

namespace
{
    bool IsPowerOfTwo(std::size_t Value)
    {
        return Value != 0 && (Value & (Value - 1)) == 0;
    }

    bool TryAlignUp(std::size_t Value, std::size_t Alignment, std::size_t& AlignedValue)
    {
        if (!IsPowerOfTwo(Alignment))
        {
            return false;
        }

        const std::size_t AlignmentMask = Alignment - 1;

        if (Value > std::numeric_limits<std::size_t>::max() - AlignmentMask)
        {
            return false;
        }

        AlignedValue = (Value + AlignmentMask) & ~AlignmentMask;
        return true;
    }

    bool TryRoundUpToMultiple(std::size_t Value, std::size_t Multiple, std::size_t& RoundedValue)
    {
        if (Multiple == 0)
        {
            return false;
        }

        const std::size_t Remainder = Value % Multiple;

        if (Remainder == 0)
        {
            RoundedValue = Value;
            return true;
        }

        const std::size_t AmountToAdd = Multiple - Remainder;

        if (Value > std::numeric_limits<std::size_t>::max() - AmountToAdd)
        {
            return false;
        }

        RoundedValue = Value + AmountToAdd;
        return true;
    }
}

MemoryArena::~MemoryArena()
{
    Release();
}

bool MemoryArena::Initialize(std::size_t InReserveSize, std::size_t InCommitChunkSize)
{
    Release();

    if (InReserveSize == 0 || InCommitChunkSize == 0)
    {
        return false;
    }

    SYSTEM_INFO SystemInfo = {};
    GetSystemInfo(&SystemInfo);

    const std::size_t PageSize = SystemInfo.dwPageSize;

    std::size_t AlignedReserveSize = 0;
    std::size_t AlignedCommitChunkSize = 0;

    if (!TryRoundUpToMultiple(InReserveSize, PageSize, AlignedReserveSize) ||
        !TryRoundUpToMultiple(InCommitChunkSize, PageSize, AlignedCommitChunkSize))
    {
        return false;
    }

    // NOTE(ljh): 여기서는 주소 공간만 reserve한다. 실제 읽기/쓰기는 EnsureCommitted 이후 가능하다.
    void* ReservedMemory = VirtualAlloc(
        nullptr,
        AlignedReserveSize,
        MEM_RESERVE,
        PAGE_NOACCESS
    );

    if (ReservedMemory == nullptr)
    {
        return false;
    }

    Base = static_cast<std::byte*>(ReservedMemory);
    ReservedSize = AlignedReserveSize;
    CommitChunkSize = AlignedCommitChunkSize;

    return true;
}

void MemoryArena::Release()
{
    if (Base != nullptr)
    {
        VirtualFree(Base, 0, MEM_RELEASE);
    }

    Base = nullptr;
    ReservedSize = 0;
    CommittedSize = 0;
    Position = 0;
    CommitChunkSize = 0;
    HighWaterMark = 0;
}

void* MemoryArena::AllocateBytes(std::size_t Size, std::size_t Alignment)
{
    if (Base == nullptr || Size == 0 || !IsPowerOfTwo(Alignment))
    {
        return nullptr;
    }

    std::size_t AlignedPosition = 0;

    if (!TryAlignUp(Position, Alignment, AlignedPosition))
    {
        return nullptr;
    }

    if (AlignedPosition > ReservedSize || Size > ReservedSize - AlignedPosition)
    {
        return nullptr;
    }

    const std::size_t RequiredSize = AlignedPosition + Size;

    if (!EnsureCommitted(RequiredSize))
    {
        return nullptr;
    }

    void* Result = Base + AlignedPosition;
    Position = RequiredSize;

    if (Position > HighWaterMark)
    {
        HighWaterMark = Position;
    }

    return Result;
}

MemoryArena::Marker MemoryArena::GetMarker() const
{
    return Position;
}

void MemoryArena::Rewind(Marker MarkerPosition)
{
    if (MarkerPosition <= Position)
    {
        Position = MarkerPosition;
    }
}

void MemoryArena::Clear()
{
    // NOTE(ljh): commit된 메모리는 유지하여 다음 프레임에서 다시 사용할 수 있게 한다.
    Position = 0;
}

bool MemoryArena::EnsureCommitted(std::size_t RequiredSize)
{
    if (RequiredSize <= CommittedSize)
    {
        return true;
    }

    if (RequiredSize > ReservedSize || Base == nullptr)
    {
        return false;
    }

    std::size_t NewCommittedSize = 0;

    if (!TryRoundUpToMultiple(
            RequiredSize,
            CommitChunkSize,
            NewCommittedSize))
    {
        return false;
    }

    if (NewCommittedSize > ReservedSize)
    {
        NewCommittedSize = ReservedSize;
    }

    const std::size_t CommitSize = NewCommittedSize - CommittedSize;
    void* CommitAddress = Base + CommittedSize;

    // NOTE(ljh): 부족한 범위만 CommitChunkSize 단위로 읽고 쓸 수 있게 만든다.
    void* CommittedMemory = VirtualAlloc(
        CommitAddress,
        CommitSize,
        MEM_COMMIT,
        PAGE_READWRITE
    );

    if (CommittedMemory == nullptr)
    {
        return false;
    }

    CommittedSize = NewCommittedSize;
    return true;
}

std::size_t MemoryArena::GetUsedSize() const
{
    return Position;
}

std::size_t MemoryArena::GetCommittedSize() const
{
    return CommittedSize;
}

std::size_t MemoryArena::GetReservedSize() const
{
    return ReservedSize;
}

std::size_t MemoryArena::GetHighWaterMark() const
{
    return HighWaterMark;
}

