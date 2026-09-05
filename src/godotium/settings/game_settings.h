#ifndef GODOTIUM_SETTINGS_GAME_SETTINGS_H_
#define GODOTIUM_SETTINGS_GAME_SETTINGS_H_

#include "godot_cpp/classes/config_file.hpp"
#include "godot_cpp/classes/node.hpp"

namespace godotium {
class GameSettings : public godot::Node {
  GDCLASS(GameSettings, godot::Node)
 public:
  static GameSettings* Get();
  void _enter_tree() override;
  void _exit_tree() override;
  void _ready() override;
  double GetMusicVolume() const { return music_volume_; }
  double GetSoundVolume() const { return sound_volume_; }
  bool IsFullscreen() const { return fullscreen_; }
  bool IsVsyncEnabled() const { return vsync_; }
  void SetLanguage(const godot::String& locale);
  void SetMusicVolume(double value);
  void SetSoundVolume(double value);
  void SetFullscreen(bool enabled);
  void SetVsync(bool enabled);
  bool IsTrackEnabled(const godot::String& id) const;
  void SetTrackEnabled(const godot::String& id, bool enabled);

 protected:
  static void _bind_methods();

 private:
  void ApplyFullscreen();
  void ApplyVsync();
  void Save(const godot::String& section,
            const godot::String& key,
            const godot::Variant& value);
  static GameSettings* instance_;
  godot::Ref<godot::ConfigFile> config_;
  double music_volume_ = 30.0;
  double sound_volume_ = 70.0;
  bool fullscreen_ = false;
  bool vsync_ = true;
  godot::String language_;
};
}  // namespace godotium
#endif  // GODOTIUM_SETTINGS_GAME_SETTINGS_H_
