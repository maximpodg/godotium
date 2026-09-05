#include "godotium/scenes/game_scene/game_scene.h"

#include <cstdint>

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/viewport.hpp"

namespace godotium {
void GameScene::_bind_methods() {
  ADD_SIGNAL(godot::MethodInfo("pause_requested"));
}

void GameScene::_ready() {
  if (!godot::Engine::get_singleton()->is_editor_hint()) {
    StartNew();
    SetRunning(true);
  }
}

void GameScene::StartNew() {
  state_ = GameState{};
  RefreshTimer();
}

bool GameScene::Restore(const GameState& state) {
  if (!state.IsValid()) {
    return false;
  }
  state_ = state;
  RefreshTimer();
  return true;
}

GameState GameScene::GetState() const {
  return state_;
}

void GameScene::SetRunning(bool running) {
  running_ = running;
  set_process(running);
}

void GameScene::_process(double delta) {
  if (running_ && state_.Advance(delta)) {
    RefreshTimer();
  }
}

godot::String GameScene::GetStateSummary(const GameState& state) {
  const auto seconds = static_cast<int64_t>(state.elapsed_seconds);
  return godot::String::num_int64(seconds / 3600).pad_zeros(2) + ":" +
         godot::String::num_int64(seconds / 60 % 60).pad_zeros(2) + ":" +
         godot::String::num_int64(seconds % 60).pad_zeros(2);
}

void GameScene::RefreshTimer() {
  get_node<godot::Label>("%TimerLabel")->set_text(GetStateSummary(state_));
}

void GameScene::_unhandled_key_input(
    const godot::Ref<godot::InputEvent>& event) {
  if (running_ && is_visible_in_tree() &&
      event->is_action_pressed("ui_cancel")) {
    get_viewport()->set_input_as_handled();
    SetRunning(false);
    emit_signal("pause_requested");
  }
}
}  // namespace godotium
