#include "godotium/scenes/settings_scene/settings_scene.h"

#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/button.hpp"
#include "godot_cpp/classes/check_button.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/h_slider.hpp"
#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/option_button.hpp"
#include "godot_cpp/classes/tab_container.hpp"
#include "godot_cpp/classes/translation.hpp"
#include "godot_cpp/classes/translation_server.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godotium/localization/localization.h"
#include "godotium/music_player/music_player.h"
#include "godotium/settings/game_settings.h"

namespace godotium {
using godot::Button;
using godot::CheckButton;
using godot::Engine;
using godot::HSlider;
using godot::Label;
using godot::OptionButton;
using godot::String;
using godot::TabContainer;
using godot::TranslationServer;

void SettingsScene::_bind_methods() {
  ADD_SIGNAL(godot::MethodInfo("closed"));
  ADD_SIGNAL(
      godot::MethodInfo("play_track_requested",
                        godot::PropertyInfo(godot::Variant::INT, "index")));
}

void SettingsScene::_ready() {
  if (Engine::get_singleton()->is_editor_hint()) {
    return;
  }
  auto* settings = GameSettings::Get();
  ERR_FAIL_NULL_MSG(settings, "GameSettings autoload is missing.");
  get_node<Button>("%TestSound")
      ->connect("pressed", callable_mp(this, &SettingsScene::PlayClick));
  auto* language = get_node<OptionButton>("%LanguageOption");
  for (const String& locale : GetLocales()) {
    const auto translation =
        TranslationServer::get_singleton()->get_translation_object(locale);
    language->add_item(translation->get_message("LANGUAGE_NAME"));
  }
  language->connect("item_selected",
                    callable_mp(this, &SettingsScene::SelectLanguage));
  get_node<Button>("%CloseSettings")
      ->connect("pressed", callable_mp(this, &SettingsScene::Close));
  auto* music_slider = get_node<HSlider>("%MusicVolume");
  auto* sound_slider = get_node<HSlider>("%SoundVolume");
  music_slider->connect("value_changed",
                        callable_mp(this, &SettingsScene::SetMusicVolume));
  sound_slider->connect("value_changed",
                        callable_mp(this, &SettingsScene::SetSoundVolume));
  get_node<CheckButton>("%Fullscreen")
      ->connect("toggled", callable_mp(this, &SettingsScene::SetFullscreen));
  get_node<CheckButton>("%Vsync")->connect(
      "toggled", callable_mp(this, &SettingsScene::SetVsync));
  BuildTracks();
  settings->connect("playlist_changed",
                    callable_mp(this, &SettingsScene::RefreshTracks));
  RefreshValues();
  RefreshLabels();
}

void SettingsScene::BuildTracks() {
  auto* rows = get_node<godot::VBoxContainer>("%TrackRows");
  const auto& tracks = MusicPlayer::GetTracks();
  for (size_t index = 0; index < tracks.size(); ++index) {
    auto* row = memnew(godot::HBoxContainer);
    row->set_custom_minimum_size(godot::Vector2(0, 36));
    rows->add_child(row);
    auto* enabled = memnew(CheckButton);
    enabled->set_name("Enabled");
    enabled->set_text(String::utf8(tracks[index].title));
    enabled->add_theme_font_size_override("font_size", 14);
    enabled->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    row->add_child(enabled);
    enabled->connect("toggled", callable_mp(this, &SettingsScene::ToggleTrack)
                                    .bind(static_cast<int64_t>(index)));
    auto* play = memnew(Button);
    play->set_name("Play");
    play->set_text(String::utf8("▶"));
    play->set_custom_minimum_size(godot::Vector2(36, 36));
    row->add_child(play);
    play->connect("pressed", callable_mp(this, &SettingsScene::RequestTrack)
                                 .bind(static_cast<int64_t>(index)));
  }
}

void SettingsScene::RefreshTracks() {
  auto* rows = get_node<godot::VBoxContainer>("%TrackRows");
  const auto& tracks = MusicPlayer::GetTracks();
  for (size_t index = 0; index < tracks.size(); ++index) {
    auto* row = rows->get_child(index);
    const bool enabled = GameSettings::Get()->IsTrackEnabled(tracks[index].id);
    row->get_node<CheckButton>("Enabled")->set_pressed_no_signal(enabled);
    auto* play = row->get_node<Button>("Play");
    play->set_disabled(!enabled);
    play->set_tooltip_text(tr("MUSIC_PLAY_TRACK") + godot::String(": ") +
                           String::utf8(tracks[index].title));
  }
}

void SettingsScene::ToggleTrack(bool enabled, int64_t index) {
  ERR_FAIL_INDEX(index, MusicPlayer::GetTracks().size());
  GameSettings::Get()->SetTrackEnabled(MusicPlayer::GetTracks()[index].id,
                                       enabled);
}

void SettingsScene::RequestTrack(int64_t index) {
  ERR_FAIL_INDEX(index, MusicPlayer::GetTracks().size());
  if (GameSettings::Get()->IsTrackEnabled(MusicPlayer::GetTracks()[index].id)) {
    emit_signal("play_track_requested", index);
  }
}

void SettingsScene::RefreshValues() {
  RefreshTracks();
  auto* settings = GameSettings::Get();
  ERR_FAIL_NULL_MSG(settings, "GameSettings autoload is missing.");
  // Another settings panel may have changed the shared preferences. Reading
  // them must not emit change signals or write the configuration back to disk.
  get_node<OptionButton>("%LanguageOption")
      ->select(
          GetLocales().find(TranslationServer::get_singleton()->get_locale()));
  const double music = settings->GetMusicVolume();
  const double sound = settings->GetSoundVolume();
  get_node<HSlider>("%MusicVolume")->set_value_no_signal(music);
  get_node<HSlider>("%SoundVolume")->set_value_no_signal(sound);
  get_node<Label>("%MusicValue")->set_text(String::num_int64(music) + "%");
  get_node<Label>("%SoundValue")->set_text(String::num_int64(sound) + "%");
  get_node<CheckButton>("%Fullscreen")
      ->set_pressed_no_signal(settings->IsFullscreen());
  get_node<CheckButton>("%Vsync")->set_pressed_no_signal(
      settings->IsVsyncEnabled());
}

void SettingsScene::PlayClick() {
  get_node<godot::AudioStreamPlayer>("%SoundPlayer")->play();
}

void SettingsScene::_unhandled_key_input(
    const godot::Ref<godot::InputEvent>& event) {
  if (is_visible_in_tree() && event->is_action_pressed("ui_cancel")) {
    get_viewport()->set_input_as_handled();
    Close();
  }
}

void SettingsScene::Open() {
  RefreshValues();
  PlayClick();
  show();
  get_node<TabContainer>("%SettingsTabs")->set_current_tab(0);
  get_node<OptionButton>("%LanguageOption")->grab_focus();
}

void SettingsScene::Close() {
  PlayClick();
  hide();
  emit_signal("closed");
}

void SettingsScene::SelectLanguage(int64_t index) {
  const auto locales = GetLocales();
  ERR_FAIL_INDEX(index, locales.size());
  GameSettings::Get()->SetLanguage(locales[index]);
}

void SettingsScene::SetMusicVolume(double value) {
  GameSettings::Get()->SetMusicVolume(value);
  get_node<Label>("%MusicValue")->set_text(String::num_int64(value) + "%");
}

void SettingsScene::SetSoundVolume(double value) {
  GameSettings::Get()->SetSoundVolume(value);
  get_node<Label>("%SoundValue")->set_text(String::num_int64(value) + "%");
}

void SettingsScene::SetFullscreen(bool enabled) {
  GameSettings::Get()->SetFullscreen(enabled);
}

void SettingsScene::SetVsync(bool enabled) {
  GameSettings::Get()->SetVsync(enabled);
}

void SettingsScene::_notification(int what) {
  if (what == NOTIFICATION_TRANSLATION_CHANGED && is_node_ready() &&
      !Engine::get_singleton()->is_editor_hint()) {
    RefreshLabels();
  }
}

void SettingsScene::RefreshLabels() {
  RefreshTracks();
  auto* tabs = get_node<TabContainer>("%SettingsTabs");
  tabs->set_tab_title(0, tr("TAB_GENERAL"));
  tabs->set_tab_title(1, tr("TAB_AUDIO"));
  tabs->set_tab_title(2, tr("TAB_GRAPHICS"));
}
}  // namespace godotium
