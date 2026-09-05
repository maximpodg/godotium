#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")" && pwd)"
build_dir="$project_root/src/out/Default"

if [[ "${1:-}" == "--editor" ]]; then
  "$project_root/build_tools.sh"
  (
    cd "$project_root/src"
    "$project_root/.tools/bin/gn" gen "$build_dir"
    "$project_root/.tools/bin/ninja" -C "$build_dir" godot game_cpp_host
  )
  python3 "$project_root/src/build/scripts/prepare_editor.py" "$build_dir"
  exec "$build_dir/bin/godot" --editor --path "$project_root/src"
fi

case "$(uname -s)" in
  Darwin) app_binary="$build_dir/dist/Godotium.app/Contents/MacOS/Godotium" ;;
  Linux) app_binary="$build_dir/dist/godotium" ;;
  *) echo "Use the executable in out/Default/dist on this platform." >&2; exit 1 ;;
esac
if [[ ! -x "$app_binary" ]]; then
  "$project_root/build.sh"
fi
exec "$app_binary" "$@"
