#!/usr/bin/env bash
# Quick end-to-end sanity check. Runs the emulator with a fast tick and a scripted session.
# This is NOT the graded test suite -- see TEAM_TASKS.md (chunk 4) for the real test matrix.
# Extend it as each chunk lands (screen -s/-r, rr, FOR, ...).
set -u
cd "$(dirname "$0")/.."
BIN=./csopesy
[ -x "$BIN" ] || { echo "build first: make"; exit 1; }

fail=0
check() { if grep -q -- "$2" "$1"; then echo "PASS: $3"; else echo "FAIL: $3"; fail=1; fi; }

OUT=$(mktemp)
(
  echo "screen -ls"            # must be rejected before initialize
  echo "initialize"
  echo "scheduler-start"
  sleep 1
  echo "screen -ls"
  echo "scheduler-stop"
  echo "report-util"
  echo "exit"
) | $BIN --tick-us 200 > "$OUT" 2>&1

check "$OUT" "Please run 'initialize' first." "commands rejected before initialize"
check "$OUT" "Initialized from"               "initialize reads config"
check "$OUT" "Scheduler started"              "scheduler-start"
check "$OUT" "CPU utilization:"               "screen -ls prints utilization"
check "$OUT" "Report generated at"            "report-util writes a file"
check "$OUT" "Finished processes:"            "report lists finished processes"
[ -s csopesy-log.txt ] && echo "PASS: csopesy-log.txt is non-empty" || { echo "FAIL: csopesy-log.txt"; fail=1; }

rm -f "$OUT"
exit $fail
