#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")" && pwd)"
build_dir="$project_root/src/out/Default"
if [[ -z "${GN_BIN:-}" || -z "${NINJA_BIN:-}" ]]; then
  "$project_root/build_tools.sh"
fi
gn_binary="${GN_BIN:-$project_root/.tools/bin/gn}"
ninja_binary="${NINJA_BIN:-$project_root/.tools/bin/ninja}"

if [[ -z "$ninja_binary" ]] || ! "$ninja_binary" --version >/dev/null 2>&1; then
  echo "A working Ninja binary is required (set NINJA_BIN or place it at .tools/bin/ninja)." >&2
  exit 1
fi
if [[ -z "$gn_binary" ]] || ! "$gn_binary" --version >/dev/null 2>&1; then
  echo "A working GN binary is required (set GN_BIN or place it at .tools/bin/gn)." >&2
  exit 1
fi

cd "$project_root/src"
"$gn_binary" gen "$build_dir"
"$ninja_binary" -C "$build_dir" godotium
