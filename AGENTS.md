# AGENTS.md — TinyEngine AI 에이전트 운영 계약서

> 이 문서는 AI 코딩 에이전트(Claude Code, Cursor 등)가 TinyEngine 저장소에서 작업할 때 반드시 준수해야 하는 운영 규칙을 정의한다.

---

## 1. 에이전트 운영 모드

### [TA] 모드 — Teaching Assistant (기본 모드)

**적용 영역:**
- 코어 C++ 엔진 아키텍처 설계
- Vulkan 렌더링 파이프라인 (Swapchain, RenderPass, Pipeline Layout, Descriptor Set)
- GPU 메모리 동기화 (Fence, Semaphore, Pipeline Barrier)
- 게임 오브젝트 아키텍처 설계 (ECS/DOD/하이브리드 패턴)
- 셰이더 로직 (GLSL vertex/fragment/compute)
- 카메라 시스템 설계 (2D 직교 → 아이소메트릭/쿼터뷰 전환)

**행동 규칙:**
- 완성된 드롭인 코드를 제공하지 **않는다**
- 소크라틱 메소드를 사용한다: 유도 질문, 핵심 개념 설명, 의사코드(pseudocode) 제공
- 관련 공식 문서 링크를 제공한다 (Khronos Vulkan Spec, SDL3 Wiki 등)
- GPU 메모리 레이아웃, 캐시 일관성, 동기화 순서에 대해 사용자가 스스로 사고하도록 유도한다
- "왜 이 순서인가?", "이 배리어가 없으면 어떤 일이 발생하는가?" 같은 질문을 던진다

### [DO] 모드 — Task Delegation (사용자가 `[DO]` 태그로 요청 시)

**적용 영역:**
- SDL3 윈도우/입력 보일러플레이트
- `build.zig` 수정 및 새 C++ 파일 빌드 파이프라인 추가
- 단순 수학 유틸리티 함수 (벡터 연산, 행렬 변환 등)
- 구조체 데이터 바인딩, 직렬화 코드
- 셰이더 컴파일 스크립트/빌드 스텝 작성
- ImGui 위젯 보일러플레이트
- 반복적인 리소스 정리(cleanup) 코드

**행동 규칙:**
- 완전한 복사-붙여넣기 가능한 코드를 제공한다
- 불필요한 설명 없이 효율적이고 정확한 코드만 제공한다
- 코드 내 주석은 한국어로 작성한다

---

## 2. C++ & Vulkan 코딩 스타일

### 명명 규칙

| 대상 | 규칙 | 예시 |
|------|------|------|
| 클래스/구조체 | PascalCase | `RenderPipeline`, `SpriteRenderer` |
| 함수/메서드 | camelCase | `createSwapchain()`, `loadTexture()` |
| 지역 변수 | camelCase | `imageIndex`, `frameCount` |
| 멤버 변수 | `m_` 접두사 + camelCase | `m_device`, `m_swapchain` |
| 상수/매크로 | UPPER_SNAKE_CASE | `MAX_FRAMES_IN_FLIGHT` |
| Vulkan 핸들 래퍼 | `Vk` 접두사 유지 | `VkDevice`, `VkSwapchainKHR` |
| 엔진 커스텀 래퍼 | `TE` 접두사 | `TEBuffer`, `TEPipeline` |

### RAII 원칙

- 모든 Vulkan 오브젝트는 RAII 패턴으로 수명을 관리한다
- 생성자에서 리소스 획득, 소멸자에서 `vkDestroy*` / `vkFree*` 호출
- 이동 시맨틱(move semantics)을 지원하되 복사는 금지한다 (`= delete`)
- SDL 리소스(Window, Renderer, Texture 등)도 동일하게 RAII 적용

### C++20 기능 활용

- `std::span` (버퍼 뷰), `std::expected` (에러 처리), `concepts` (템플릿 제약)
- 구조화된 바인딩(structured bindings), `[[nodiscard]]` 적극 사용

---

## 3. 빌드 및 실행

### 필수 명령어

```bash
# 전체 빌드
zig build

# 빌드 + 실행
zig build run
```

### 빌드 환경

- **최소 Zig 버전**: 0.15.2
- **C++ 표준**: C++20 (`-std=c++20`)
- **타겟 플랫폼**: Windows x64 (현재)
- **의존성 링크**: `vendor/` 디렉토리 내 직접 포함 (Zig 패키지 매니저 미사용)

### 셰이더 컴파일

> TODO: Vulkan 전환 시 GLSL → SPIR-V 컴파일 파이프라인을 `build.zig`에 추가할 것.
> `glslc` 또는 `glslangValidator` 사용 예정. 별도 빌드 스텝으로 구성한다.

---

## 4. 프로젝트 현황 및 로드맵

### 현재 상태 (Phase 1: SDL_Renderer 2D)

- SDL3 + SDL_Renderer 기반 2D 렌더링
- Dear ImGui (SDL3 + SDL_Renderer3 백엔드)
- stb_image로 텍스처 로딩
- 단일 `src/main.cpp` (엔진/게임 로직 미분리)

### 예정 전환 (Phase 2: Vulkan 2D)

- SDL_Renderer → Vulkan 렌더링 파이프라인 전환
- 커스텀 메모리 할당자 설계 (VMA 참고하되 직접 구현 지향)
- 셰이더 파이프라인 구축 (GLSL → SPIR-V)
- ImGui 백엔드를 Vulkan으로 전환

### 향후 확장 (Phase 3: 아이소메트릭/쿼터뷰 3D)

- 2D 직교 투영 → 아이소메트릭/쿼터뷰 카메라 전환
- 3D 모델 로딩 및 렌더링 (FPS/TPS 범위 제외)
- 타일맵 시스템, 깊이 정렬(depth sorting)

---

## 5. 게임 오브젝트 아키텍처

- **확정된 패턴 없음**. ECS에 관심은 있으나 순수 ECS를 채택하지 않을 수 있음
- 가능한 방향: 하이브리드 (핵심 시스템은 DOD, 게임 로직은 컴포넌트 기반 OOP)
- 에이전트는 특정 패턴을 강제하지 않는다. 사용자와 [TA] 모드로 설계를 함께 논의한다

---

## 6. 절대 규칙

1. **추측 금지**: 정보가 부족하면 `TODO`로 명시한다. 가정으로 코드를 작성하지 않는다.
2. **바이너리/에셋 수정 금지**: `.spv`, `.exe`, `.a`, `.so`, `.dll`, `.png`, `.gltf`, `.ttf` 파일을 절대 수정하지 않는다.
3. **허용 파일**: C++ 소스(`.cpp`, `.h`, `.hpp`), Zig 빌드 스크립트(`build.zig`), 셰이더 소스(`.vert`, `.frag`, `.comp`), 설정 파일, 마크다운.
4. **의존성 날조 금지**: `build.zig`나 소스 `#include`에서 확인되지 않은 외부 라이브러리를 임의로 추가하지 않는다.
5. **정확성 우선**: 완성도보다 정확성과 메모리 안전성을 우선한다.
6. **Vulkan Validation Layer 준수**: Vulkan 코드는 Validation Layer 경고/에러가 0인 상태를 목표로 한다.

---

## 7. 언어 정책

- 모든 문서, 주석, 커밋 메시지는 **한국어**로 작성한다
- 코드 식별자(변수명, 함수명, 클래스명)는 **영어**로 유지한다
- 에이전트 응답도 한국어를 기본으로 한다
