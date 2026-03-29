# Phase 1 구조화 계획

## 목표
main.cpp 단일 파일 PoC → Vulkan 전환에 살아남는 모듈화된 엔진 기반

## 실행 순서
- [x] Step 0: .gitignore, 미사용 파일 삭제, test.png→assets/ 이동
- [ ] Step 1: TEWindow, TEInput, TEClock (SDL3 영구 추상화)
- [ ] Step 5: build.zig 멀티파일 + 에셋 복사
- [ ] Step 2A: TEEngine (고정 시간 스텝 게임 루프)
- [ ] Step 4A: TEImGui (백엔드 격리)
- [ ] Step 2B: TERenderer(가상 인터페이스) + TERendererSDL
- [ ] Step 3: TETexture + 에셋 경로
- [ ] Step 6: main.cpp 리팩토링 (~10줄)

## 설계 결정 완료
- VS 프로젝트 파일: 유지 (.gitignore에 추가)
- 렌더러: 순수 가상 클래스 (TERenderer → TERendererSDL/TERendererVulkan)
- 네이밍: `TE` 접두사 (`TEWindow`, `TEBuffer` 등)
- 에러 처리: 치명적 에러는 조기 종료, 복구 불필요
- 소유권: RAII 소유 + raw 포인터로 빌려줌 (수명 보장 시)

## Phase 1에서 만들지 않는 것
스프라이트 배칭, 씬 그래프, 엔티티 시스템, 커스텀 할당자, 카메라, 셰이더
