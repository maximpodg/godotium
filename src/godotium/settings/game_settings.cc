#include "godotium/settings/game_settings.h"

#include <algorithm>
#include <cmath>

#include "godot_cpp/classes/audio_server.hpp"
#include "godot_cpp/classes/display_server.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/translation_server.hpp"
#include "godotium/localization/localization.h"

namespace godotium {
namespace {
godot::String GetSettingsPath() {
  const godot::String path =
      godot::OS::get_singleton()->get_environment("GODOTIUM_SETTINGS_PATH");
  return path.is_empty() ? godot::String("user://settings.cfg") : path;
}
double NormalizeVolume(double value, double fallback) {
  return std::isfinite(value) ? std::clamp(value, 0.0, 100.0) : fallback;
}
void ApplyVolume(const char* bus, double value) {
  auto* audio = godot::AudioServer::get_singleton();
  const int index = audio->get_bus_index(bus);
  ERR_FAIL_COND_MSG(index < 0, "Missing configured audio bus.");
  audio->set_bus_volume_linear(index, value / 100.0);
  audio->set_bus_mute(index, value <= 0);
}
}  // namespace

void GameSettings::_bind_methods() {
  ADD_SIGNAL(godot::MethodInfo("playlist_changed"));
}

bool GameSettings::IsTrackEnabled(const godot::String& id) const {
  const auto value = config_->get_value("music_tracks", id, true);
  return value.get_type() == godot::Variant::BOOL ? static_cast<bool>(value)
                                                  : true;
}

void GameSettings::SetTrackEnabled(const godot::String& id, bool enabled) {
  Save("music_tracks", id, enabled);
  emit_signal("playlist_changed");
}

GameSettings* GameSettings::instance_ = nullptr;
GameSettings* GameSettings::Get() {
  return instance_;
}
void GameSettings::_enter_tree() {
  if (!godot::Engine::get_singleton()->is_editor_hint()) {
    instance_ = this;
  }
}
void GameSettings::_exit_tree() {
  if (instance_ == this) {
    instance_ = nullptr;
  }
}
void GameSettings::_ready() {
  if (godot::Engine::get_singleton()->is_editor_hint()) {
    return;
  }
  config_.instantiate();
  const auto error = config_->load(GetSettingsPath());
  if (error != godot::OK && error != godot::ERR_FILE_NOT_FOUND) {
    WARN_PRINT("Could not load game settings; using defaults.");
    config_->clear();
  }
  auto* translations = godot::TranslationServer::get_singleton();
  language_ = FindSupportedLocale(
      config_->get_value("interface", "language", translations->get_locale()));
  music_volume_ =
      NormalizeVolume(config_->get_value("audio", "music", 30.0), 30.0);
  sound_volume_ =
      NormalizeVolume(config_->get_value("audio", "sound", 70.0), 70.0);
  fullscreen_ = config_->get_value("graphics", "fullscreen", false);
  vsync_ = config_->get_value("graphics", "vsync", true);
  translations->set_locale(language_);
  ApplyVolume("Music", GetMusicVolume());
  ApplyVolume("SFX", GetSoundVolume());
  ApplyFullscreen();
  ApplyVsync();
}
void GameSettings::Save(const godot::String& section,
                        const godot::String& key,
                        const godot::Variant& value) {
  config_->set_value(section, key, value);
  if (config_->save(GetSettingsPath()) != godot::OK) {
    WARN_PRINT("Could not save game settings.");
  }
}
void GameSettings::SetLanguage(const godot::String& locale) {
  const auto supported = FindSupportedLocale(locale);
  if (language_ != supported) {
    language_ = supported;
    godot::TranslationServer::get_singleton()->set_locale(supported);
  }
  Save("interface", "language", supported);
}
void GameSettings::SetMusicVolume(double value) {
  const double normalized = NormalizeVolume(value, 30.0);
  if (music_volume_ != normalized) {
    music_volume_ = normalized;
    ApplyVolume("Music", music_volume_);
  }
  Save("audio", "music", GetMusicVolume());
}
void GameSettings::SetSoundVolume(double value) {
  const double normalized = NormalizeVolume(value, 70.0);
  if (sound_volume_ != normalized) {
    sound_volume_ = normalized;
    ApplyVolume("SFX", sound_volume_);
  }
  Save("audio", "sound", GetSoundVolume());
}
void GameSettings::ApplyFullscreen() {
  auto* display = godot::DisplayServer::get_singleton();
  if (display->get_name() == "headless") {
    return;
  }
  display->window_set_mode(IsFullscreen()
                               ? godot::DisplayServer::WINDOW_MODE_FULLSCREEN
                               : godot::DisplayServer::WINDOW_MODE_WINDOWED);
}
void GameSettings::ApplyVsync() {
  auto* display = godot::DisplayServer::get_singleton();
  if (display->get_name() == "headless") {
    return;
  }
  display->window_set_vsync_mode(IsVsyncEnabled()
                                     ? godot::DisplayServer::VSYNC_ENABLED
                                     : godot::DisplayServer::VSYNC_DISABLED);
}
void GameSettings::SetFullscreen(bool enabled) {
  if (fullscreen_ != enabled) {
    fullscreen_ = enabled;
    ApplyFullscreen();
  }
  Save("graphics", "fullscreen", enabled);
}
void GameSettings::SetVsync(bool enabled) {
  if (vsync_ != enabled) {
    vsync_ = enabled;
    ApplyVsync();
  }
  Save("graphics", "vsync", enabled);
}
}  // namespace godotium
