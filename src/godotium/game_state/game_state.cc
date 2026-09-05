#include "godotium/game_state/game_state.h"

#include <cmath>

namespace godotium {
bool GameState::IsValid() const {
  return std::isfinite(elapsed_seconds) && elapsed_seconds >= 0.0 &&
         elapsed_seconds <= kMaxElapsedSeconds;
}

bool GameState::Advance(double delta_seconds) {
  if (!IsValid() || !std::isfinite(delta_seconds) || delta_seconds < 0.0 ||
      delta_seconds > kMaxElapsedSeconds - elapsed_seconds) {
    return false;
  }
  elapsed_seconds += delta_seconds;
  return true;
}
}  // namespace godotium
