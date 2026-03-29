# 80 — C++ 스타일 가이드

## C++ 표준

- **C++20** (`-std=c++20`)
- 활용 권장 기능: `std::span`, `std::expected`, `concepts`, 구조화된 바인딩, `[[nodiscard]]`

## 명명 규칙

| 대상 | 규칙 | 예시 |
|------|------|------|
| 클래스/구조체 | PascalCase | `RenderPipeline`, `SpriteRenderer` |
| 함수/메서드 | camelCase | `createSwapchain()`, `loadTexture()` |
| 지역 변수 | camelCase | `imageIndex`, `frameCount` |
| 멤버 변수 | `m_` 접두사 + camelCase | `m_device`, `m_swapchain` |
| 상수 | UPPER_SNAKE_CASE | `MAX_FRAMES_IN_FLIGHT` |
| 매크로 | UPPER_SNAKE_CASE | `TINY_ENGINE_VERSION` |
| 네임스페이스 | lowercase | `tiny`, `tiny::render` |
| 열거형 (enum class) | PascalCase (타입), PascalCase (값) | `enum class BufferUsage { Vertex, Index }` |
| 파일명 | PascalCase | `RenderPipeline.h`, `RenderPipeline.cpp` |

## 엔진 커스텀 접두사

- 엔진 래퍼 클래스: `Te` 접두사 — `TeBuffer`, `TePipeline`, `TeImage`
- Vulkan 네이티브 핸들은 `Vk` 접두사 유지 — `VkDevice`, `VkSwapchainKHR`

## RAII 원칙

```cpp
// 올바른 예: RAII로 Vulkan 리소스 관리
class TeBuffer {
public:
    TeBuffer(VkDevice device, /* ... */);
    ~TeBuffer();                          // vkDestroyBuffer + vkFreeMemory
    TeBuffer(TeBuffer&& other) noexcept;  // 이동 생성자
    TeBuffer& operator=(TeBuffer&& other) noexcept;
    TeBuffer(const TeBuffer&) = delete;   // 복사 금지
    TeBuffer& operator=(const TeBuffer&) = delete;
private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};
```

## 헤더 파일 규칙

- `#pragma once` 사용 (include guard 대신)
- 포함 순서:
  1. 대응하는 헤더 (`Foo.cpp` → `"Foo.h"`)
  2. 프로젝트 헤더 (`"engine/Renderer.h"`)
  3. 서드파티 헤더 (`<SDL3/SDL.h>`, `<glm/glm.hpp>`)
  4. 표준 라이브러리 (`<vector>`, `<memory>`)

## 구조체 패딩 및 정렬

- Vulkan UBO(Uniform Buffer Object)는 `std140` 정렬 규칙 준수
- `alignas(16)` 등으로 명시적 정렬 지정
- `vec3`는 16바이트 정렬이 필요함을 주의 (패딩 이슈)

```cpp
// std140 정렬 예시
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
```

## 에러 처리

- Vulkan 호출 결과는 항상 `VkResult`를 확인한다
- 치명적 에러는 `std::cerr` + 조기 종료
- TODO: `std::expected` 기반 에러 처리 패턴 도입 검토

## .clang-format (TODO)

> 아직 `.clang-format` 파일이 없다. 필요 시 생성한다.
> 기본 방향:
> - 들여쓰기: 4 스페이스
> - 중괄호: 같은 줄 (K&R 스타일)
> - 열 제한: 120자
> - 포인터/참조: 타입 쪽 (`int* ptr`, `const std::string& name`)

## 주석 규칙

- 코드 주석은 **한국어**로 작성
- 코드 식별자는 **영어** 유지
- 함수 문서화: 필요한 경우에만, 간결하게
