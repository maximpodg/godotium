#ifndef GODOTIUM_SCENES_SETTINGS_SCENE_SETTINGS_SCENE_H_
#define GODOTIUM_SCENES_SETTINGS_SCENE_SETTINGS_SCENE_H_

#include <cstdint>

#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/panel_container.hpp"

namespace godotium {
class SettingsScene : public godot::PanelContainer {
  GDCLASS(SettingsScene, godot::PanelContainer)
 public:
  void _ready() override;
  void _unhandled_key_input(
      const godot::Ref<godot::InputEvent>& event) override;
  void Open();
  void Close();

 protected:
  static void _bind_methods();
  void _notification(int what);

 private:
  void BuildTracks();
  void RefreshTracks();
  void ToggleTrack(bool enabled, int64_t index);
  void RequestTrack(int64_t index);
  void RefreshValues();
  void RefreshLabels();
  void PlayClick();
  void SelectLanguage(int64_t index);
  void SetMusicVolume(double value);
  void SetSoundVolume(double value);
  void SetFullscreen(bool enabled);
  void SetVsync(bool enabled);
};
}  // namespace godotium
#endif  // GODOTIUM_SCENES_SETTINGS_SCENE_SETTINGS_SCENE_H_
