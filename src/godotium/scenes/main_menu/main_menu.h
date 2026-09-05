#ifndef GODOTIUM_SCENES_MAIN_MENU_MAIN_MENU_H_
#define GODOTIUM_SCENES_MAIN_MENU_MAIN_MENU_H_

#include <cstdint>

#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/panel_container.hpp"

namespace godotium {
class MainMenu : public godot::PanelContainer {
  GDCLASS(MainMenu, godot::PanelContainer)
 public:
  enum class Action { kNewGame, kLoad, kSave, kResume, kSettings, kQuit };

  void _ready() override;
  void _unhandled_key_input(
      const godot::Ref<godot::InputEvent>& event) override;
  void Open(bool can_resume);
  void FocusAction(Action action);
  void SetStatus(godot::String key);

 protected:
  static void _bind_methods();
  void _notification(int what);

 private:
  void RequestAction(int64_t action);
  void RefreshLabels();
  bool can_resume_ = false;
  godot::String status_key_;
};
}  // namespace godotium
#endif  // GODOTIUM_SCENES_MAIN_MENU_MAIN_MENU_H_
