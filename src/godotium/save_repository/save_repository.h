#ifndef GODOTIUM_SAVE_REPOSITORY_SAVE_REPOSITORY_H_
#define GODOTIUM_SAVE_REPOSITORY_SAVE_REPOSITORY_H_

#include <cstdint>
#include <vector>

#include "godot_cpp/variant/string.hpp"
#include "godotium/game_state/game_state.h"

namespace godotium {
struct SavedGame {
  godot::String id;
  godot::String name;
  GameState state;
  int64_t modified = 0;
  bool valid = false;
};

// File storage and serialization only. No UI, translations or scene lookups.
class SaveRepository {
 public:
  enum class SaveResult { kSaved, kNeedsOverwrite, kInvalidName, kFailed };

  std::vector<SavedGame> List() const;
  bool Delete(const godot::String& id) const;
  bool Load(const godot::String& id, SavedGame& result) const;
  SaveResult Save(const godot::String& name,
                  const GameState& state,
                  bool overwrite) const;
};
}  // namespace godotium
#endif  // GODOTIUM_SAVE_REPOSITORY_SAVE_REPOSITORY_H_
