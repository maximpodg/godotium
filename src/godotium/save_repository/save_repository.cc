#include "godotium/save_repository/save_repository.h"

#include <algorithm>

#include "godot_cpp/classes/config_file.hpp"
#include "godot_cpp/classes/dir_access.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/time.hpp"

namespace godotium {
namespace {
// Version of the on-disk schema, independent of the game's release version.
// Incompatible schema changes need a new version and explicit load migration.
constexpr int kSaveFormatVersion = 1;

godot::String SaveDirectory() {
  const auto path =
      godot::OS::get_singleton()->get_environment("GODOTIUM_SAVE_DIR");
  return path.is_empty() ? godot::String("user://saves") : path;
}

godot::String NamedSavePath(const godot::String& name) {
  // Display names never become filesystem paths. Case variants share a slot.
  return SaveDirectory().path_join(name.to_lower().sha256_text() + ".cfg");
}
}  // namespace

bool SaveRepository::Load(const godot::String& id, SavedGame& result) const {
  result = SavedGame{};
  result.id = id;
  result.modified = godot::FileAccess::get_modified_time(id);
  godot::Ref<godot::ConfigFile> file;
  file.instantiate();
  if (file->load(id) != godot::OK) {
    return false;
  }
  const auto name = file->get_value("game", "name", "");
  if (name.get_type() == godot::Variant::STRING) {
    result.name = name;
  }
  if (file->get_value("game", "version", 0) !=
      godot::Variant(kSaveFormatVersion)) {
    return false;
  }
  const auto elapsed =
      file->get_value("game", "elapsed_seconds", godot::Variant());
  if (elapsed.get_type() != godot::Variant::FLOAT &&
      elapsed.get_type() != godot::Variant::INT) {
    return false;
  }
  result.state.elapsed_seconds = elapsed;
  result.valid = result.state.IsValid();
  return result.valid;
}

bool SaveRepository::Delete(const godot::String& id) const {
  return godot::DirAccess::remove_absolute(id) == godot::OK;
}

std::vector<SavedGame> SaveRepository::List() const {
  std::vector<SavedGame> entries;
  const auto directory = SaveDirectory();
  if (godot::DirAccess::dir_exists_absolute(directory)) {
    const auto dir = godot::DirAccess::open(directory);
    if (dir.is_valid()) {
      for (const auto& file : dir->get_files()) {
        if (file.ends_with(".cfg")) {
          SavedGame entry;
          Load(directory.path_join(file), entry);
          entries.push_back(entry);
        }
      }
    }
  }
  // Existing single-slot saves remain available without a file migration.
  auto legacy =
      godot::OS::get_singleton()->get_environment("GODOTIUM_SAVE_PATH");
  if (legacy.is_empty() && godot::OS::get_singleton()
                               ->get_environment("GODOTIUM_SAVE_DIR")
                               .is_empty()) {
    legacy = "user://savegame.cfg";
  }
  if (!legacy.is_empty() && godot::FileAccess::file_exists(legacy)) {
    SavedGame entry;
    Load(legacy, entry);
    entries.push_back(entry);
  }
  std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
    return a.modified != b.modified ? a.modified > b.modified : a.id < b.id;
  });
  return entries;
}

SaveRepository::SaveResult SaveRepository::Save(
    const godot::String& requested_name,
    const GameState& state,
    bool overwrite) const {
  const auto name = requested_name.strip_edges();
  if (name.is_empty() || name.length() > 80) {
    return SaveResult::kInvalidName;
  }
  if (!state.IsValid()) {
    return SaveResult::kFailed;
  }
  const auto path = NamedSavePath(name);
  if (!overwrite && godot::FileAccess::file_exists(path)) {
    return SaveResult::kNeedsOverwrite;
  }
  godot::Ref<godot::ConfigFile> file;
  file.instantiate();
  file->set_value("game", "version", kSaveFormatVersion);
  file->set_value("game", "name", name);
  file->set_value("game", "elapsed_seconds", state.elapsed_seconds);
  file->set_value("game", "saved_at",
                  godot::Time::get_singleton()->get_unix_time_from_system());
  const auto temporary = path + godot::String(".tmp");
  // Finish writing before replacing the old slot.
  if (godot::DirAccess::make_dir_recursive_absolute(SaveDirectory()) !=
          godot::OK ||
      file->save(temporary) != godot::OK ||
      godot::DirAccess::rename_absolute(temporary, path) != godot::OK) {
    return SaveResult::kFailed;
  }
  return SaveResult::kSaved;
}
}  // namespace godotium
