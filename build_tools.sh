#!/usr/bin/env bash
# May be sourced from Bash or Zsh. Keep shell options and traps unchanged.
_godotium_build_tools() {
  local script_path project_root
  if [ -n "${ZSH_VERSION:-}" ]; then
    eval 'script_path="${(%):-%x}"'
  elif [ -n "${BASH_VERSION:-}" ]; then
    script_path="${BASH_SOURCE[0]}"
  else
    echo "Use Bash or Zsh to source build_tools.sh." >&2
    return 1
  fi
  project_root="$(cd "$(dirname "$script_path")" && pwd)" || return 1
  python3 "$project_root/src/build/scripts/setup_tools.py" || return 1
  case ":$PATH:" in
    ":$project_root/.tools/bin:"*) ;;
    *) export PATH="$project_root/.tools/bin:$PATH" ;;
  esac
  echo "GN and Ninja are ready in $project_root/.tools/bin."
}

if _godotium_build_tools; then
  unset -f _godotium_build_tools
else
  unset -f _godotium_build_tools
  return 1 2>/dev/null || exit 1
fi
