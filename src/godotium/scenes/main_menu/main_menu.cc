#include "godotium/scenes/main_menu/main_menu.h"

#include <array>

#include "godot_cpp/classes/button.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"

namespace godotium {
namespace {
struct ActionInfo {
  const char* node;
  const char* signal;
  const char* label;
};
constexpr std::array<ActionInfo, 6> kActions = {{
    {"%NewGameButton", "new_game_requested", "MENU_NEW_GAME"},
    {"%LoadGameButton", "load_requested", "MENU_LOAD_GAME"},
    {"%SaveGameButton", "save_requested", "MENU_SAVE_GAME"},
    {"%ResumeGameButton", "resume_requested", "MENU_RESUME"},
    {"%SettingsButton", "settings_requested", "MENU_SETTINGS"},
    {"%QuitButton", "quit_requested", "MENU_EXIT"},
}};
}  // namespace

void MainMenu::_bind_methods() {
  for (const auto& action : kActions) {
    ADD_SIGNAL(godot::MethodInfo(action.signal));
  }
}

void MainMenu::_ready() {
  if (godot::Engine::get_singleton()->is_editor_hint()) {
    return;
  }
  for (size_t index = 0; index < kActions.size(); ++index) {
    get_node<godot::Button>(kActions[index].node)
        ->connect("pressed", callable_mp(this, &MainMenu::RequestAction)
                                 .bind(static_cast<int64_t>(index)));
  }
  const godot::String version =
      godot::ProjectSettings::get_singleton()->get_setting(
          "application/config/version", "");
  auto* label = get_node<godot::Label>("%VersionLabel");
  label->set_text(godot::String("v") + version);
  label->set_visible(!version.is_empty());
  RefreshLabels();
}

void MainMenu::Open(bool can_resume) {
  can_resume_ = can_resume;
  get_node<godot::Button>("%ResumeGameButton")->set_visible(can_resume);
  get_node<godot::Button>("%SaveGameButton")->set_visible(can_resume);
  show();
  FocusAction(can_resume ? Action::kResume : Action::kNewGame);
}

void MainMenu::FocusAction(Action action) {
  get_node<godot::Button>(kActions[static_cast<size_t>(action)].node)
      ->grab_focus();
}

void MainMenu::RequestAction(int64_t action) {
  ERR_FAIL_INDEX(action, kActions.size());
  emit_signal(kActions[action].signal);
}

void MainMenu::_unhandled_key_input(
    const godot::Ref<godot::InputEvent>& event) {
  if (can_resume_ && is_visible_in_tree() &&
      event->is_action_pressed("ui_cancel")) {
    get_viewport()->set_input_as_handled();
    emit_signal("resume_requested");
  }
}

void MainMenu::SetStatus(godot::String key) {
  status_key_ = key;
  auto* label = get_node<godot::Label>("%GameStatus");
  label->set_text(key.is_empty() ? godot::String() : tr(key));
  label->set_visible(!key.is_empty());
}

void MainMenu::RefreshLabels() {
  for (const auto& action : kActions) {
    get_node<godot::Button>(action.node)->set_text(tr(action.label));
  }
  SetStatus(status_key_);
}

void MainMenu::_notification(int what) {
  if (what == NOTIFICATION_TRANSLATION_CHANGED && is_node_ready() &&
      !godot::Engine::get_singleton()->is_editor_hint()) {
    RefreshLabels();
  }
}
}  // namespace godotium
