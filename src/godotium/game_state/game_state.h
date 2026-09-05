#ifndef GODOTIUM_GAME_STATE_GAME_STATE_H_
#define GODOTIUM_GAME_STATE_GAME_STATE_H_

namespace godotium {
// Persistent demo state and simulation rules; independent of engine and
// storage.
struct GameState {
  static constexpr double kMaxElapsedSeconds = 1.0e12;
  double elapsed_seconds = 0.0;

  bool IsValid() const;
  // Invalid or out-of-range steps leave the state unchanged.
  bool Advance(double delta_seconds);
};
}  // namespace godotium
#endif  // GODOTIUM_GAME_STATE_GAME_STATE_H_
