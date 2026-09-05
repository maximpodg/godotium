#include "godotium/music_player/music_player.h"

#include "godot_cpp/classes/audio_stream.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godotium/settings/game_settings.h"

namespace godotium {
const std::array<MusicPlayer::Track, 2>& MusicPlayer::GetTracks() {
  static constexpr std::array<Track, 2> tracks = {{
      {"out_in_space_menu", "Agecaf · Out in Space — Menu (CC0)",
       "res://godotium/resources/audio/music/out_in_space_menu.ogg"},
      {"out_in_space", "Agecaf · Out in Space (CC0)",
       "res://godotium/resources/audio/music/out_in_space.ogg"},
  }};
  return tracks;
}

void MusicPlayer::_ready() {
  if (godot::Engine::get_singleton()->is_editor_hint()) {
    return;
  }
  auto* settings = GameSettings::Get();
  ERR_FAIL_NULL(settings);
  settings->connect("playlist_changed",
                    callable_mp(this, &MusicPlayer::PlaylistChanged));
  connect("finished", callable_mp(this, &MusicPlayer::PlayNext));
  PlayNext();
}

void MusicPlayer::PlayTrack(int64_t index) {
  const auto& tracks = GetTracks();
  ERR_FAIL_INDEX(index, tracks.size());
  if (!GameSettings::Get()->IsTrackEnabled(tracks[index].id)) {
    return;
  }
  godot::Ref<godot::AudioStream> track =
      godot::ResourceLoader::get_singleton()->load(tracks[index].path);
  ERR_FAIL_COND_MSG(track.is_null(), "Could not load a music track.");
  current_track_ = index;
  set_stream(track);
  play();
}

void MusicPlayer::PlayNext() {
  const auto& tracks = GetTracks();
  for (size_t offset = 1; offset <= tracks.size(); ++offset) {
    const int index =
        (current_track_ + static_cast<int>(offset)) % tracks.size();
    if (GameSettings::Get()->IsTrackEnabled(tracks[index].id)) {
      PlayTrack(index);
      return;
    }
  }
  stop();
  current_track_ = -1;
}

void MusicPlayer::PlaylistChanged() {
  if (current_track_ < 0 ||
      !GameSettings::Get()->IsTrackEnabled(GetTracks()[current_track_].id)) {
    PlayNext();
  }
}
}  // namespace godotium
