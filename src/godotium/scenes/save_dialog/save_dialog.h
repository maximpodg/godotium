#ifndef GODOTIUM_SCENES_SAVE_DIALOG_SAVE_DIALOG_H_
#define GODOTIUM_SCENES_SAVE_DIALOG_SAVE_DIALOG_H_

#include <cstdint>
#include <vector>

#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/panel_container.hpp"

namespace godotium {
// A reusable name/overwrite form. Storage is provided by its signal consumer.
class SaveDialog : public godot::PanelContainer {
  GDCLASS(SaveDialog, godot::PanelContainer)
 public:
  // Named slots supplied by the coordinator; the form never reads files.
  struct Entry {
    godot::String name;
    godot::String summary;
    godot::String date;
    bool valid = false;
  };

  void _ready() override;
  void _input(const godot::Ref<godot::InputEvent>& event) override;
  void Open(const godot::String& suggested_name,
            const std::vector<Entry>& entries);
  void MarkNameTaken(const godot::String& name);
  void SetStatus(godot::String key);

 protected:
  static void _bind_methods();
  void _notification(int what);

 private:
  void Cancel();
  void Confirm();
  void Overwrite();
  void FocusDefault();
  void NameChanged(const godot::String& name);
  void SubmitName(const godot::String& name);
  void RefreshLabels();
  void SelectSave(int64_t index);
  void RefreshEntries();
  std::vector<Entry> entries_;
  bool name_exists_ = false;
  godot::String status_key_;
};
}  // namespace godotium
#endif  // GODOTIUM_SCENES_SAVE_DIALOG_SAVE_DIALOG_H_
