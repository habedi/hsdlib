#!/usr/bin/env bash
set -euo pipefail

# This script runs all targets in the Makefile and logs the output to a file.
echo "== Running the tests for the Makefile targets =="

# List of all targets
TARGETS=(
  "python-setup" "example" "bench-aarch64" "bench-amd64" "bench-clean"  "bench"
  "build" "rebuild" "test" "test-aarch64" "test-amd64"
  "cov" "doc" "format" "help" "install-deps" "lint"
  "build-release" "python-build" "python-install" "python-publish"
  "python-test" "python-clean"
  "clean"
)

# Targets to skip
SKIP_TARGETS=(
  #"clean" "rebuild" "cov" "python-clean"
  "install-deps" #"python-install" "build-release"
  #"python-build" "python-test"
  "bench-amd64" "python-publish"
  #"bench-aarch64" "bench-clean"
  #"test-amd64"
  "test-aarch64" "bench"
)

LOG_DIR="make_test_logs"
mkdir -p "$LOG_DIR"

# Helper function that checks if value is present in an array
contains() {
  local val="$1"; shift
  for x in "$@"; do [[ "$x" == "$val" ]] && return 0; done
  return 1
}

printf "%-20s %-8s %-10s\n" "target" "status" "duration"
printf "%-20s %-8s %-10s\n" "------" "------" "--------"

for tgt in "${TARGETS[@]}"; do
  if contains "$tgt" "${SKIP_TARGETS[@]}"; then
    printf "%-20s %-8s %-10s\n" "$tgt" "skipped" "-"
    continue
  fi

  log_file="$LOG_DIR/$tgt.log"
  start=$(date +%s.%N)

  if make "$tgt" >"$log_file" 2>&1; then
    status="ok"
  else
    status="fail"
  fi

  end=$(date +%s.%N)
  # Calculate runtime in seconds with ms
  duration=$(printf "%.3f" "$(echo "$end - $start" | bc)")

  printf "%-20s %-8s %-10s\n" "$tgt" "$status" "${duration}s"
done
