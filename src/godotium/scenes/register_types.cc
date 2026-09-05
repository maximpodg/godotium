#include "godotium/scenes/register_types.h"

#include "godot_cpp/godot.hpp"
#include "godotium/scenes/game_scene/game_scene.h"
#include "godotium/scenes/load_dialog/load_dialog.h"
#include "godotium/scenes/main_menu/main_menu.h"
#include "godotium/scenes/main_scene/main_scene.h"
#include "godotium/scenes/save_dialog/save_dialog.h"
#include "godotium/scenes/settings_scene/settings_scene.h"

namespace godotium {
void RegisterSceneTypes() {
  GDREGISTER_CLASS(GameScene);
  GDREGISTER_CLASS(MainMenu);
  GDREGISTER_CLASS(SaveDialog);
  GDREGISTER_CLASS(LoadDialog);
  GDREGISTER_CLASS(MainScene);
  GDREGISTER_CLASS(SettingsScene);
}
}  // namespace godotium
