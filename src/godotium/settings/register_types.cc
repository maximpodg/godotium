#include "godotium/settings/register_types.h"

#include "godot_cpp/godot.hpp"
#include "godotium/settings/game_settings.h"

namespace godotium {
void RegisterSettingsTypes() {
  GDREGISTER_CLASS(GameSettings);
}
}  // namespace godotium
