# 50 — 메모리 관리 및 동기화

> **상태**: 미구현. Vulkan 전환 시 핵심 설계 영역.
> 이 영역은 반드시 [TA] 모드로 다룬다.

## 메모리 할당 전략

### 사용자 방향

- **커스텀 할당자를 직접 구현**하는 것을 지향
- VMA(Vulkan Memory Allocator)를 참고하되 그대로 사용하지 않을 예정
- Vulkan 메모리 관리에 대한 학습이 목적에 포함됨

### Vulkan 메모리 기본 개념

```
VkPhysicalDeviceMemoryProperties
├── memoryTypes[]     # 각 타입의 속성 (HOST_VISIBLE, DEVICE_LOCAL 등)
└── memoryHeaps[]     # 물리적 메모리 힙 (VRAM, System RAM)
```

> **핵심 질문 (TA 모드용):**
> - `HOST_VISIBLE | HOST_COHERENT`와 `DEVICE_LOCAL`의 성능 차이는?
> - Staging Buffer 패턴이 필요한 이유는?
> - 메모리 타입 선택 시 `memoryTypeBits`를 어떻게 활용하는가?

### 할당자 설계 고려사항 (TODO)

- **서브 할당(Sub-allocation)**: `vkAllocateMemory` 호출 최소화, 큰 블록을 나눠 사용
- **메모리 풀링**: 용도별 풀 (버텍스 버퍼용, 텍스처용, 스테이징용)
- **조각화(Fragmentation) 관리**: 프리 리스트, 버디 할당 등
- **정렬(Alignment)**: `bufferImageGranularity`, `minUniformBufferOffsetAlignment` 준수

## 동기화 프리미티브

### 개요

| 프리미티브 | 용도 | 범위 |
|-----------|------|------|
| Fence | CPU ↔ GPU 동기화 | CPU가 GPU 작업 완료를 대기 |
| Semaphore | GPU ↔ GPU 동기화 | 큐 간 실행 순서 제어 |
| Pipeline Barrier | 커맨드 내 동기화 | 이미지 레이아웃 전환, 메모리 접근 순서 |
| Event | 세밀한 GPU 동기화 | 커맨드 버퍼 내 신호 |

### 프레임 동기화 패턴 (TODO)

```
Frames-in-Flight (일반적으로 2~3):
  - 각 프레임: 자체 Command Buffer + Fence + Semaphore 세트
  - imageAvailableSemaphore: Swapchain 이미지 획득 신호
  - renderFinishedSemaphore: 렌더링 완료 신호
  - inFlightFence: CPU가 이전 프레임 완료를 대기
```

> **핵심 질문 (TA 모드용):**
> - Fence 없이 렌더링하면 어떤 문제가 발생하는가?
> - `MAX_FRAMES_IN_FLIGHT`를 2로 설정하는 이유는?
> - `vkWaitForFences`와 `vkResetFences`의 호출 순서가 중요한 이유는?

## Command Buffer 관리 (TODO)

- Command Pool: 스레드당 하나
- Command Buffer 녹화(Recording) → 제출(Submit) → 리셋(Reset) 사이클
- 일회성 커맨드: `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`

## 참고 자료

- [Vulkan Memory Management (GPU Open)](https://gpuopen.com/learn/vulkan-memory-management/)
- [Vulkan Synchronization (Khronos Blog)](https://www.khronos.org/blog/understanding-vulkan-synchronization)
- [VMA 소스 코드 (참고용)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
