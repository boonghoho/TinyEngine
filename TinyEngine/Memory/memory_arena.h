#pragma once

#include <cstddef>
#include <limits>

// REFERENCE(ljh): Memory arena 학습 및 구현 참고 자료
// - Ryan Fleury memory management talk: https://www.youtube.com/watch?v=UeJPyuVxL-o
// - RAD Debugger arena implementation: https://github.com/EpicGames/raddebugger/blob/master/src/base/base_arena.c

namespace tiny
{

class MemoryArena
{
public:
    using Marker = std::size_t;

    MemoryArena() = default;
    ~MemoryArena();

    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;

    bool Initialize(std::size_t ReserveSize, std::size_t CommitChunkSize);

    void Release();

    // NOTE(ljh): 정렬된 raw memory만 반환한다. 객체의 생성자와 소멸자는 호출하지 않는다.
    void* AllocateBytes(std::size_t Size, std::size_t Alignment = alignof(std::max_align_t));

	// NOTE(ljh): 정렬된 객체의 raw memory만 반환한다. 객체의 생성자와 소멸자는 호출하지 않는다.
    template<typename T>
    T* AllocateArray(std::size_t Count)
    {
        if (Count == 0 ||Count > std::numeric_limits<std::size_t>::max() / sizeof(T))
        {
            return nullptr;
        }

        return static_cast<T*>(AllocateBytes(sizeof(T) * Count, alignof(T)));
	}

    Marker GetMarker() const;

    // NOTE(ljh): Rewind와 Clear는 할당 위치만 되돌린다. 객체의 소멸자는 호출하지 않는다.
    void Rewind(Marker MarkerPosition);
    void Clear();

    std::size_t GetUsedSize() const;
    std::size_t GetCommittedSize() const;
    std::size_t GetReservedSize() const;

    std::size_t GetHighWaterMark() const;

private:
    bool EnsureCommitted(std::size_t RequiredSize);

    std::byte* Base = nullptr;

    std::size_t ReservedSize = 0;
    std::size_t CommittedSize = 0;

    std::size_t Position = 0;

    std::size_t CommitChunkSize = 0;

    std::size_t HighWaterMark = 0;
};

}

