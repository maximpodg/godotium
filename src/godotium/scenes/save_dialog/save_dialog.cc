#include "godotium/scenes/save_dialog/save_dialog.h"

#include "godot_cpp/classes/button.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/item_list.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/line_edit.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"

namespace godotium {
void SaveDialog::_bind_methods() {
  ADD_SIGNAL(godot::MethodInfo(
      "save_requested", godot::PropertyInfo(godot::Variant::STRING, "name"),
      godot::PropertyInfo(godot::Variant::BOOL, "overwrite")));
  ADD_SIGNAL(godot::MethodInfo("cancelled"));
}

void SaveDialog::_ready() {
  if (godot::Engine::get_singleton()->is_editor_hint()) {
    return;
  }
  get_node<godot::Button>("%CancelButton")
      ->connect("pressed", callable_mp(this, &SaveDialog::Cancel));
  get_node<godot::Button>("%ConfirmButton")
      ->connect("pressed", callable_mp(this, &SaveDialog::Confirm));
  get_node<godot::Button>("%OverwriteButton")
      ->connect("pressed", callable_mp(this, &SaveDialog::Overwrite));
  get_node<godot::LineEdit>("%SaveName")
      ->connect("text_changed", callable_mp(this, &SaveDialog::NameChanged));
  get_node<godot::LineEdit>("%SaveName")
      ->connect("text_submitted", callable_mp(this, &SaveDialog::SubmitName));
  get_node<godot::ItemList>("%SaveList")
      ->connect("item_selected", callable_mp(this, &SaveDialog::SelectSave));
  RefreshEntries();
  NameChanged(get_node<godot::LineEdit>("%SaveName")->get_text());
}

void SaveDialog::Open(const godot::String& suggested_name,
                      const std::vector<Entry>& entries) {
  entries_ = entries;
  RefreshEntries();
  auto* name = get_node<godot::LineEdit>("%SaveName");
  name->set_text(suggested_name);
  NameChanged(suggested_name);
  show();
  FocusDefault();
}

void SaveDialog::RefreshEntries() {
  auto* list = get_node<godot::ItemList>("%SaveList");
  const auto selection = list->get_selected_items();
  list->clear();
  for (const auto& entry : entries_) {
    list->add_item(entry.name + godot::String("\n") +
                   (entry.valid ? entry.summary : tr("SAVE_DAMAGED")) +
                   "  |  " + entry.date);
  }
  if (!selection.is_empty() && selection[0] < list->get_item_count()) {
    list->select(selection[0]);
  }
  list->set_visible(!entries_.empty());
  get_node<godot::Label>("%ExistingSavesLabel")
      ->set_text(tr(entries_.empty() ? "SAVE_EMPTY" : "SAVE_EXISTING_HINT"));
}

void SaveDialog::SelectSave(int64_t index) {
  ERR_FAIL_INDEX(index, entries_.size());
  const auto name = entries_[index].name;
  get_node<godot::LineEdit>("%SaveName")->set_text(name);
  NameChanged(name);
  FocusDefault();
}

void SaveDialog::NameChanged(const godot::String& name) {
  name_exists_ = false;
  auto* list = get_node<godot::ItemList>("%SaveList");
  list->deselect_all();
  for (size_t index = 0; index < entries_.size(); ++index) {
    if (entries_[index].name.to_lower() == name.strip_edges().to_lower()) {
      list->select(index);
      name_exists_ = true;
      break;
    }
  }
  const auto trimmed = name.strip_edges();
  const bool valid = !trimmed.is_empty() && trimmed.length() <= 80;
  auto* save = get_node<godot::Button>("%ConfirmButton");
  auto* cancel = get_node<godot::Button>("%CancelButton");
  const bool update_focus = save->has_focus() || cancel->has_focus();
  save->set_disabled(!valid || name_exists_);
  get_node<godot::Button>("%OverwriteButton")
      ->set_disabled(!valid || !name_exists_);
  if (update_focus) {
    FocusDefault();
  }
  SetStatus("");
  RefreshLabels();
}

void SaveDialog::FocusDefault() {
  const bool can_save =
      !get_node<godot::Button>("%ConfirmButton")->is_disabled();
  get_node<godot::Button>(can_save ? "%ConfirmButton" : "%CancelButton")
      ->grab_focus();
}

void SaveDialog::SubmitName(const godot::String& name) {
  // Enter in the name field follows the default action, never overwrites.
  if (get_node<godot::Button>("%ConfirmButton")->is_disabled()) {
    Cancel();
  } else {
    Confirm();
  }
}

void SaveDialog::Confirm() {
  if (get_node<godot::Button>("%ConfirmButton")->is_disabled()) {
    return;
  }
  emit_signal("save_requested",
              get_node<godot::LineEdit>("%SaveName")->get_text().strip_edges(),
              false);
}

void SaveDialog::Overwrite() {
  if (get_node<godot::Button>("%OverwriteButton")->is_disabled()) {
    return;
  }
  emit_signal("save_requested",
              get_node<godot::LineEdit>("%SaveName")->get_text().strip_edges(),
              true);
}

void SaveDialog::MarkNameTaken(const godot::String& name) {
  // A slot may have appeared since the form opened. Refresh availability rather
  // than silently turning a create request into an overwrite.
  entries_.push_back({name, "", "", false});
  RefreshEntries();
  NameChanged(get_node<godot::LineEdit>("%SaveName")->get_text());
  FocusDefault();
}

void SaveDialog::Cancel() {
  hide();
  name_exists_ = false;
  emit_signal("cancelled");
}

void SaveDialog::_input(const godot::Ref<godot::InputEvent>& event) {
  // LineEdit consumes Esc; handle cancellation before it receives the key.
  if (!godot::Engine::get_singleton()->is_editor_hint() &&
      is_visible_in_tree() && event->is_action_pressed("ui_cancel")) {
    get_viewport()->set_input_as_handled();
    Cancel();
  }
}

void SaveDialog::SetStatus(godot::String key) {
  status_key_ = key;
  auto* label = get_node<godot::Label>("%StatusLabel");
  label->set_text(key.is_empty() ? godot::String() : tr(key));
  label->set_visible(!key.is_empty());
}

void SaveDialog::RefreshLabels() {
  SetStatus(status_key_);
}

void SaveDialog::_notification(int what) {
  if (what == NOTIFICATION_TRANSLATION_CHANGED && is_node_ready() &&
      !godot::Engine::get_singleton()->is_editor_hint()) {
    RefreshEntries();
    RefreshLabels();
  }
}
}  // namespace godotium
