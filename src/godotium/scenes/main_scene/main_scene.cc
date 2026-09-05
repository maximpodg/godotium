#include "godotium/scenes/main_scene/main_scene.h"

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godotium/music_player/music_player.h"
#include "godotium/scenes/game_scene/game_scene.h"
#include "godotium/scenes/load_dialog/load_dialog.h"
#include "godotium/scenes/main_menu/main_menu.h"
#include "godotium/scenes/save_dialog/save_dialog.h"
#include "godotium/scenes/settings_scene/settings_scene.h"

namespace godotium {
void MainScene::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("QuitGame"),
                              &MainScene::QuitGame);
}

void MainScene::_ready() {
  if (godot::Engine::get_singleton()->is_editor_hint()) {
    return;
  }
  game_ = get_node<GameScene>("%GameScene");
  menu_ = get_node<MainMenu>("%MainMenu");
  settings_ = get_node<SettingsScene>("%SettingsScene");
  save_dialog_ = get_node<SaveDialog>("%SaveDialog");
  load_dialog_ = get_node<LoadDialog>("%LoadDialog");
  menu_->connect("new_game_requested", callable_mp(this, &MainScene::NewGame));
  menu_->connect("resume_requested", callable_mp(this, &MainScene::ResumeGame));
  menu_->connect("load_requested", callable_mp(this, &MainScene::OpenLoad));
  menu_->connect("save_requested", callable_mp(this, &MainScene::OpenSave));
  menu_->connect("settings_requested",
                 callable_mp(this, &MainScene::OpenSettings));
  menu_->connect("quit_requested", callable_mp(this, &MainScene::QuitGame));
  game_->connect("pause_requested", callable_mp(this, &MainScene::ShowMenu));
  settings_->connect(
      "closed", callable_mp(this, &MainScene::ReturnToMenu)
                    .bind(static_cast<int64_t>(MainMenu::Action::kSettings)));
  save_dialog_->connect(
      "cancelled", callable_mp(this, &MainScene::ReturnToMenu)
                       .bind(static_cast<int64_t>(MainMenu::Action::kSave)));
  load_dialog_->connect(
      "cancelled", callable_mp(this, &MainScene::ReturnToMenu)
                       .bind(static_cast<int64_t>(MainMenu::Action::kLoad)));
  save_dialog_->connect("save_requested",
                        callable_mp(this, &MainScene::SaveGame));
  load_dialog_->connect("load_requested",
                        callable_mp(this, &MainScene::LoadGame));
  load_dialog_->connect("delete_requested",
                        callable_mp(this, &MainScene::DeleteGame));
  settings_->connect("play_track_requested",
                     callable_mp(get_node<MusicPlayer>("%MusicPlayer"),
                                 &MusicPlayer::PlayTrack));
  ShowMenu();
}

void MainScene::ShowScreen(Screen screen) {
  screen_ = screen;
  game_->SetRunning(screen == Screen::kGame);
  game_->set_visible(screen == Screen::kGame);
  menu_->set_visible(screen == Screen::kMenu);
  settings_->set_visible(screen == Screen::kSettings);
  save_dialog_->set_visible(screen == Screen::kSave);
  load_dialog_->set_visible(screen == Screen::kLoad);
}

void MainScene::ShowMenu() {
  ShowScreen(Screen::kMenu);
  menu_->Open(has_game_);
}

void MainScene::ReturnToMenu(int64_t action) {
  ShowMenu();
  menu_->FocusAction(static_cast<MainMenu::Action>(action));
}

void MainScene::NewGame() {
  game_->StartNew();
  has_game_ = true;
  current_save_name_ = "";
  ResumeGame();
}

void MainScene::ResumeGame() {
  if (has_game_) {
    menu_->SetStatus("");
    ShowScreen(Screen::kGame);
  }
}

void MainScene::OpenSettings() {
  ShowScreen(Screen::kSettings);
  settings_->Open();
}

void MainScene::OpenSave() {
  if (has_game_) {
    ShowScreen(Screen::kSave);
    std::vector<SaveDialog::Entry> entries;
    for (const auto& save : saves_.List()) {
      // The legacy unnamed slot can be loaded, then saved under a new name.
      if (save.name.is_empty()) {
        continue;
      }
      const auto date =
          godot::Time::get_singleton()->get_datetime_string_from_unix_time(
              save.modified, true) +
          " UTC";
      entries.push_back({save.name,
                         save.valid ? GameScene::GetStateSummary(save.state)
                                    : godot::String(),
                         date, save.valid});
    }
    save_dialog_->Open(current_save_name_, entries);
  }
}

void MainScene::OpenLoad() {
  std::vector<LoadDialog::Entry> entries;
  for (const auto& save : saves_.List()) {
    const auto date =
        godot::Time::get_singleton()->get_datetime_string_from_unix_time(
            save.modified, true) +
        " UTC";
    entries.push_back(
        {save.id, save.name,
         save.valid ? GameScene::GetStateSummary(save.state) : godot::String(),
         date, save.valid});
  }
  ShowScreen(Screen::kLoad);
  load_dialog_->Open(entries);
}

void MainScene::SaveGame(const godot::String& name, bool overwrite) {
  if (screen_ != Screen::kSave || !has_game_) {
    return;
  }
  switch (saves_.Save(name, game_->GetState(), overwrite)) {
    case SaveRepository::SaveResult::kSaved:
      current_save_name_ = name.strip_edges();
      ReturnToMenu(static_cast<int64_t>(MainMenu::Action::kSave));
      menu_->SetStatus("GAME_SAVED");
      break;
    case SaveRepository::SaveResult::kNeedsOverwrite:
      save_dialog_->MarkNameTaken(name);
      break;
    case SaveRepository::SaveResult::kInvalidName:
      save_dialog_->SetStatus("SAVE_NAME_REQUIRED");
      break;
    case SaveRepository::SaveResult::kFailed:
      save_dialog_->SetStatus("GAME_SAVE_FAILED");
      break;
  }
}

void MainScene::LoadGame(const godot::String& id) {
  if (screen_ != Screen::kLoad) {
    return;
  }
  SavedGame save;
  if (!saves_.Load(id, save) || !game_->Restore(save.state)) {
    load_dialog_->SetStatus("GAME_LOAD_FAILED");
    return;
  }
  current_save_name_ = save.name;
  has_game_ = true;
  ResumeGame();
}

void MainScene::DeleteGame(const godot::String& id) {
  if (screen_ != Screen::kLoad) {
    return;
  }
  if (!saves_.Delete(id)) {
    load_dialog_->SetStatus("SAVE_DELETE_FAILED");
    return;
  }
  OpenLoad();
}

void MainScene::QuitGame() {
  get_tree()->quit();
}
}  // namespace godotium
