# 코드 인덱스

## 소스
- `src/main.cpp` — 엔진 진입점 (SDL3+ImGui+stb_image, 단일 파일 130줄)

## 빌드
- `build.zig` — Zig 빌드 설정 (C++20, SDL3 링크, ImGui 컴파일, DLL 복사)
- `build.zig.zon` — Zig 패키지 매니페스트 (min 0.15.2, 외부 dep 없음)

## 에이전트 문서
- `CLAUDE.md` — 기본 지침 + 워크플로우 규칙
- `AGENTS.md` — TA/DO 모드, 코딩 스타일, 절대 규칙
- `SKILLS/00_overview.md` — 아키텍처 개요, 게임 루프, 의존성 맵
- `SKILLS/10_vulkan_pipeline.md` — Vulkan 전환 로드맵 (TODO)
- `SKILLS/20_entity_logic.md` — 게임 오브젝트 패턴 (미확정)
- `SKILLS/30_build_and_shaders.md` — build.zig 구조, 셰이더 파이프라인 (TODO)
- `SKILLS/40_asset_management.md` — 텍스처 로딩, Vulkan 업로드 흐름
- `SKILLS/50_memory_and_sync.md` — 메모리/동기화 (TODO)
- `SKILLS/80_style_guide.md` — C++20 스타일, RAII, 네이밍

## 에셋
- `assets/test.png` — 테스트 이미지 (루트에서 이동됨)
