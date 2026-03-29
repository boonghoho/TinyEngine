# 10 — Vulkan 렌더링 파이프라인

> **상태**: 미구현. 이 문서는 Vulkan 전환 시 참조할 구조를 정리한다.
> Vulkan 관련 작업은 [TA] 모드가 기본이다.

## 전환 로드맵

1. Vulkan Instance + Physical/Logical Device 생성
2. SDL3 Vulkan Surface 연동 (`SDL_Vulkan_CreateSurface`)
3. Swapchain 생성 및 이미지 뷰 구성
4. RenderPass 또는 Dynamic Rendering 선택
5. Graphics Pipeline 생성 (셰이더 모듈, Vertex Input, Viewport, Rasterizer)
6. Command Buffer 녹화 및 제출
7. 프레임 동기화 (Fence + Semaphore)

## Swapchain

> TODO: Swapchain 설정 (Present Mode, Surface Format, Extent) 결정 필요.
> - `VK_PRESENT_MODE_FIFO_KHR` (V-Sync) vs `VK_PRESENT_MODE_MAILBOX_KHR` (Triple Buffering)
> - Surface Format: `VK_FORMAT_B8G8R8A8_SRGB` 권장

## RenderPass vs Dynamic Rendering

> TODO: Vulkan 1.3의 Dynamic Rendering (`VK_KHR_dynamic_rendering`)을 사용할지,
> 전통적 RenderPass를 사용할지 결정 필요.
> - Dynamic Rendering: 더 간결, Framebuffer 객체 불필요
> - RenderPass: 더 명시적, 서브패스 의존성 제어 가능

## Descriptor Set 설계

> TODO: 유니폼 버퍼, 텍스처 샘플러 바인딩 전략 결정 필요.
> - Per-frame vs Per-material vs Per-object descriptor set 분리
> - Descriptor Pool 크기 및 할당 전략

## Pipeline Layout

> TODO: Push Constants vs Uniform Buffer 사용 범위 결정.
> - 작은 변환 데이터 (MVP 행렬): Push Constants 고려
> - 큰 데이터 셋: Uniform Buffer Object (UBO)

## 참고 자료

- [Vulkan Specification (Khronos)](https://registry.khronos.org/vulkan/specs/latest/html/)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [SDL3 Vulkan 문서](https://wiki.libsdl.org/SDL3/CategoryVulkan)
