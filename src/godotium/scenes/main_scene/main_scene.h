#ifndef GODOTIUM_SCENES_MAIN_SCENE_MAIN_SCENE_H_
#define GODOTIUM_SCENES_MAIN_SCENE_MAIN_SCENE_H_

#include <cstdint>

#include "godot_cpp/classes/node2d.hpp"
#include "godotium/save_repository/save_repository.h"

namespace godotium {
class GameScene;
class MainMenu;
class SaveDialog;
class LoadDialog;
class SettingsScene;

// Composition root: connects scene contracts and routes persistence requests.
class MainScene : public godot::Node2D {
  GDCLASS(MainScene, godot::Node2D)
 public:
  void _ready() override;
  void QuitGame();

 protected:
  static void _bind_methods();

 private:
  enum class Screen { kMenu, kGame, kSettings, kSave, kLoad };
  void ShowScreen(Screen screen);
  void ShowMenu();
  void ReturnToMenu(int64_t action);
  void NewGame();
  void ResumeGame();
  void OpenSettings();
  void OpenSave();
  void OpenLoad();
  void SaveGame(const godot::String& name, bool overwrite);
  void LoadGame(const godot::String& id);
  void DeleteGame(const godot::String& id);

  GameScene* game_ = nullptr;
  MainMenu* menu_ = nullptr;
  SettingsScene* settings_ = nullptr;
  SaveDialog* save_dialog_ = nullptr;
  LoadDialog* load_dialog_ = nullptr;
  SaveRepository saves_;
  Screen screen_ = Screen::kMenu;
  bool has_game_ = false;
  godot::String current_save_name_;
};
}  // namespace godotium
#endif  // GODOTIUM_SCENES_MAIN_SCENE_MAIN_SCENE_H_
