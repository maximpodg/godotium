#ifndef GODOTIUM_SCENES_LOAD_DIALOG_LOAD_DIALOG_H_
#define GODOTIUM_SCENES_LOAD_DIALOG_LOAD_DIALOG_H_

#include <cstdint>
#include <vector>

#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/panel_container.hpp"

namespace godotium {
class LoadDialog : public godot::PanelContainer {
  GDCLASS(LoadDialog, godot::PanelContainer)
 public:
  // Presentation data. The dialog treats ids as opaque and never opens files.
  struct Entry {
    godot::String id;
    godot::String name;
    godot::String summary;
    godot::String date;
    bool available = false;
  };

  void _ready() override;
  void _input(const godot::Ref<godot::InputEvent>& event) override;
  void Open(const std::vector<Entry>& entries);
  void SetStatus(godot::String key);

 protected:
  static void _bind_methods();
  void _notification(int what);

 private:
  void Cancel();
  void Confirm();
  void Select(int64_t index);
  void Activate(int64_t index);
  void RefreshEntries();
  void RequestDelete();
  void ConfirmDelete();
  void CancelDelete();
  void RefreshDelete();
  godot::String delete_id_;
  Entry delete_entry_;
  std::vector<Entry> entries_;
  godot::String status_key_;
};
}  // namespace godotium
#endif  // GODOTIUM_SCENES_LOAD_DIALOG_LOAD_DIALOG_H_
