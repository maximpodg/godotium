#ifndef GODOTIUM_SCENES_GAME_SCENE_GAME_SCENE_H_
#define GODOTIUM_SCENES_GAME_SCENE_GAME_SCENE_H_

#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "godotium/game_state/game_state.h"

namespace godotium {
// Replace this scene and GameState to implement a different game.
class GameScene : public godot::VBoxContainer {
  GDCLASS(GameScene, godot::VBoxContainer)
 public:
  void _ready() override;
  void _process(double delta) override;
  void _unhandled_key_input(
      const godot::Ref<godot::InputEvent>& event) override;

  void StartNew();
  bool Restore(const GameState& state);
  GameState GetState() const;
  static godot::String GetStateSummary(const GameState& state);
  void SetRunning(bool running);

 protected:
  static void _bind_methods();

 private:
  void RefreshTimer();
  GameState state_;
  bool running_ = false;
};
}  // namespace godotium
#endif  // GODOTIUM_SCENES_GAME_SCENE_GAME_SCENE_H_
