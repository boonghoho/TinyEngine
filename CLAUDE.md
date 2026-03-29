# TinyEngine — CLAUDE.md

> 이 파일은 Claude Code가 TinyEngine 저장소에서 작업할 때 참조하는 기본 지침이다.
> 상세한 에이전트 운영 규칙은 [AGENTS.md](AGENTS.md)를 참조한다.

## 워크플로우: 코드 작성 전 반드시 탐색

**모든 작업 전에 `.claude/` 디렉토리를 먼저 확인한다.**

| 순서 | 경로 | 확인 내용 |
|------|------|----------|
| 1 | `.claude/plan/` | 현재 목표·요구사항 |
| 2 | `.claude/progress/` | 작업 진행 상황·문맥 |
| 3 | `.claude/result/` | 기존 코드 패턴·인덱스 (유사 패턴으로 작성) |
| 4 | `.claude/error/` | 과거 동일·유사 에러 및 해결책 |

**출력 규칙:**
- `.md` 기록은 경로·명령어·결과만 최소한으로
- 전체 코드베이스를 읽지 말고 `result/code_index.md`로 타겟 파일만 읽기
- `.claude/skills/`에 관련 스킬이 있는지 먼저 확인 후 작업

## 프로젝트 개요

C++20 / SDL3 기반 2D 게임 엔진. Zig 빌드 시스템 사용.
2D 렌더링에서 시작하여 Vulkan 전환 후 아이소메트릭/쿼터뷰 3D까지 확장 예정.

## 에이전트 모드 (요약)

- **[TA] 모드** (기본): 코어 아키텍처, Vulkan, 메모리 관리 — 소크라틱 메소드, 코드 직접 제공 금지
- **[DO] 모드** (`[DO]` 태그 시): 보일러플레이트, 빌드 설정 — 완전한 코드 제공

> 전체 규칙: [AGENTS.md](AGENTS.md) 참조

## 빌드 및 실행

```bash
zig build        # 전체 빌드
zig build run    # 빌드 + 실행
```

## 기술 스택

| 구분 | 기술 |
|------|------|
| 언어 | C++20 |
| 빌드 | Zig 0.15.2+ (`build.zig`) |
| 윈도우/입력 | SDL3 |
| 렌더링 | SDL_Renderer (현재) → Vulkan (예정) |
| GUI | Dear ImGui (SDL3 + SDL_Renderer3 백엔드) |
| 수학 | GLM |
| 이미지 | stb_image |

## 프로젝트 구조

```
TinyEngine/
├── AGENTS.md          # 에이전트 운영 계약서 (모드, 코딩 스타일, 절대 규칙)
├── SKILLS/            # 엔진 기술 문서
│   ├── 00_overview.md         # 엔진 아키텍처 개요
│   ├── 10_vulkan_pipeline.md  # Vulkan 파이프라인 (TODO)
│   ├── 20_entity_logic.md     # 게임 오브젝트 설계 (TODO)
│   ├── 30_build_and_shaders.md # 빌드 시스템 & 셰이더
│   ├── 40_asset_management.md  # 에셋 관리
│   ├── 50_memory_and_sync.md   # 메모리 & 동기화 (TODO)
│   └── 80_style_guide.md       # C++ 스타일 가이드
├── build.zig          # Zig 빌드 설정
├── build.zig.zon      # Zig 패키지 매니페스트
├── src/
│   └── main.cpp       # 엔진 진입점 (현재 단일 파일)
├── include/           # 헤더 파일
├── vendor/            # 서드파티 (SDL3, GLM, ImGui, stb)
├── assets/            # 게임 에셋
└── zig-out/           # 빌드 출력
```

## 핵심 원칙

1. **추측 금지** — 정보 부족 시 TODO로 명시
2. **정확성 우선** — 완성도보다 정확성, 메모리 안전성
3. **바이너리/에셋 수정 금지** — `.spv`, `.exe`, `.dll`, `.png`, `.gltf` 등 절대 불가
4. **의존성 날조 금지** — `build.zig` 또는 `#include`에서 확인된 것만 사용
5. **Vulkan Validation Layer 준수** — 경고/에러 0 목표

## 언어 정책

- 문서, 주석, 커밋 메시지: **한국어**
- 코드 식별자: **영어**
