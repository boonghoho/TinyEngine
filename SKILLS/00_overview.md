# 00 — 엔진 아키텍처 개요

## TinyEngine 소개

C++20 / SDL3 기반 2D 게임 엔진. Zig 빌드 시스템 사용.
2D 렌더링에서 시작하여 아이소메트릭/쿼터뷰 3D까지 확장을 목표로 한다.

## 렌더링 범위

| 단계 | 렌더링 방식 | 카메라 | 상태 |
|------|------------|--------|------|
| Phase 1 | SDL_Renderer 2D | 직교 투영 (Orthographic) | **현재** |
| Phase 2 | Vulkan 2D | 직교 투영 | 예정 |
| Phase 3 | Vulkan 아이소메트릭/쿼터뷰 | 아이소메트릭 투영 | 예정 |

> FPS/TPS 범위의 3D 렌더링은 계획에 포함되지 않음.

## 메인 게임 루프 (현재 구조)

```
SDL_Init → Window 생성 → Renderer 생성 → ImGui 초기화 → 에셋 로딩
    │
    ▼
┌─────────────────── Game Loop ───────────────────┐
│  1. SDL_PollEvent (입력 처리)                      │
│  2. ImGui NewFrame                                │
│  3. ImGui 위젯 업데이트 (게임 로직 UI)               │
│  4. SDL_RenderClear (화면 클리어)                   │
│  5. 게임 오브젝트 렌더링 (SDL_RenderTexture)         │
│  6. ImGui 렌더링                                   │
│  7. SDL_RenderPresent (화면 표시)                   │
└─────────────────────────────────────────────────┘
    │
    ▼
리소스 정리 → SDL_Quit
```

## 소스 구조 (현재)

```
src/
└── main.cpp          # 진입점 + 전체 엔진/게임 로직 (단일 파일)
```

> 엔진 코어와 게임 로직이 아직 분리되지 않았다.
> Vulkan 전환 시 모듈 분리가 필요하다.

### 예상 모듈 분리 구조 (TODO)

```
src/
├── main.cpp              # 진입점
├── engine/
│   ├── Window.h/cpp      # SDL3 윈도우 래퍼
│   ├── Renderer.h/cpp    # Vulkan 렌더러 코어
│   ├── Pipeline.h/cpp    # 그래픽스 파이프라인
│   └── Input.h/cpp       # 입력 시스템
├── game/
│   ├── Scene.h/cpp       # 씬/레벨 관리
│   └── Entity.h/cpp      # 게임 오브젝트
└── utils/
    └── Math.h            # 수학 유틸리티
```

> 위 구조는 참고용이며 확정이 아니다. 설계는 [TA] 모드로 논의한다.

## 의존성 맵

```
TinyEngine
├── SDL3           # 윈도우, 입력, (현재) 렌더링
├── Dear ImGui     # 디버그 UI / 에디터 UI
│   └── backends: imgui_impl_sdl3 + imgui_impl_sdlrenderer3
├── GLM            # 수학 (벡터, 행렬, 변환)
└── stb_image      # 이미지 로딩 (PNG, JPG 등)
```

> Vulkan SDK는 Phase 2에서 추가 예정.
