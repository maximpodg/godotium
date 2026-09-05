#ifndef GODOTIUM_MUSIC_PLAYER_MUSIC_PLAYER_H_
#define GODOTIUM_MUSIC_PLAYER_MUSIC_PLAYER_H_
#include <array>
#include <cstdint>

#include "godot_cpp/classes/audio_stream_player.hpp"
namespace godotium {
class MusicPlayer : public godot::AudioStreamPlayer {
  GDCLASS(MusicPlayer, godot::AudioStreamPlayer)
 public:
  struct Track {
    const char* id;
    const char* title;
    const char* path;
  };
  static const std::array<Track, 2>& GetTracks();
  void _ready() override;
  void PlayTrack(int64_t index);

 protected:
  static void _bind_methods() {}

 private:
  void PlayNext();
  void PlaylistChanged();
  int current_track_ = -1;
};
}  // namespace godotium
#endif  // GODOTIUM_MUSIC_PLAYER_MUSIC_PLAYER_H_
