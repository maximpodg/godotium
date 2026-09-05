#include "godotium/music_player/register_types.h"

#include "godot_cpp/godot.hpp"
#include "godotium/music_player/music_player.h"

namespace godotium {
void RegisterMusicPlayerTypes() {
  GDREGISTER_CLASS(MusicPlayer);
}
}  // namespace godotium
