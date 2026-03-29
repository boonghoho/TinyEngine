# 진행 상황

## 현재 단계
Step 1 대기 중 — TEWindow 설계 (TA 모드)

## 완료
- Step 0: .gitignore, main.zig/root.zig 삭제, test.png→assets/ 이동
- GitHub push: https://github.com/boonghoho/TinyEngine
- 프로젝트 세팅: CLAUDE.md, AGENTS.md, SKILLS/, .claude/ 추적 파일, SessionStart hook

## TEWindow 설계 진행 (TA 대화)
- 생성 실패 → 치명적 에러, 조기 종료 ✓
- 소유권 → RAII 소유, raw 포인터로 빌려줌 (수명 보장 시) ✓
- 설정 → TEWindowConfig 구조체, 기본값 제공 ✓
- 다음: TEWindow 코드 작성 (사용자 직접 or [DO])
