#!/bin/bash
# 세션 시작 시 .claude/ 파일 상태 요약만 주입 (내용은 필요할 때 직접 Read)
cd "$(git rev-parse --show-toplevel 2>/dev/null)" 2>/dev/null || exit 0

check_file() {
  if [ -s "$1" ]; then
    local lines=$(wc -l < "$1" | tr -d ' ')
    local mod=$(date -r "$1" "+%m-%d %H:%M" 2>/dev/null || stat -c "%y" "$1" 2>/dev/null | cut -c6-16)
    echo "$1 (${lines}줄, ${mod})"
  else
    echo "$1 (비어있음)"
  fi
}

SUMMARY=""
SUMMARY="$SUMMARY\n- $(check_file .claude/plan/current_plan.md)"
SUMMARY="$SUMMARY\n- $(check_file .claude/progress/status.md)"
SUMMARY="$SUMMARY\n- $(check_file .claude/result/code_index.md)"
SUMMARY="$SUMMARY\n- $(check_file .claude/error/issue_log.md)"

export SUMMARY
node -e "
const s = process.env.SUMMARY;
const msg = '[.claude 컨텍스트 파일 상태] 작업 전 용도에 맞는 파일을 Read로 확인할 것.\n' + s;
console.log(JSON.stringify({ systemMessage: msg }));
" 2>/dev/null || echo '{"systemMessage": "[.claude 컨텍스트] 파일 상태 확인 실패. 직접 Read할 것."}'
