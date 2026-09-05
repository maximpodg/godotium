#include "godotium/scenes/load_dialog/load_dialog.h"

#include "godot_cpp/classes/button.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/item_list.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"

namespace godotium {
void LoadDialog::_bind_methods() {
  ADD_SIGNAL(godot::MethodInfo(
      "load_requested", godot::PropertyInfo(godot::Variant::STRING, "id")));
  ADD_SIGNAL(godot::MethodInfo(
      "delete_requested", godot::PropertyInfo(godot::Variant::STRING, "id")));
  ADD_SIGNAL(godot::MethodInfo("cancelled"));
}

void LoadDialog::_ready() {
  if (godot::Engine::get_singleton()->is_editor_hint()) {
    return;
  }
  get_node<godot::Button>("%CancelButton")
      ->connect("pressed", callable_mp(this, &LoadDialog::Cancel));
  get_node<godot::Button>("%ConfirmButton")
      ->connect("pressed", callable_mp(this, &LoadDialog::Confirm));
  get_node<godot::ItemList>("%SaveList")
      ->connect("item_selected", callable_mp(this, &LoadDialog::Select));
  get_node<godot::ItemList>("%SaveList")
      ->connect("item_activated", callable_mp(this, &LoadDialog::Activate));
  get_node<godot::Button>("%DeleteButton")
      ->connect("pressed", callable_mp(this, &LoadDialog::RequestDelete));
  get_node<godot::Button>("%ConfirmDeleteButton")
      ->connect("pressed", callable_mp(this, &LoadDialog::ConfirmDelete));
  get_node<godot::Button>("%CancelDeleteButton")
      ->connect("pressed", callable_mp(this, &LoadDialog::CancelDelete));
  RefreshEntries();
  RefreshDelete();
}

void LoadDialog::Open(const std::vector<Entry>& entries) {
  delete_id_ = "";
  delete_entry_ = Entry{};
  RefreshDelete();
  entries_ = entries;
  get_node<godot::ItemList>("%SaveList")->clear();
  RefreshEntries();
  SetStatus(entries.empty() ? "SAVE_EMPTY" : "");
  show();
  if (entries.empty()) {
    get_node<godot::Button>("%CancelButton")->grab_focus();
  } else {
    get_node<godot::ItemList>("%SaveList")->grab_focus();
  }
}

void LoadDialog::RefreshEntries() {
  auto* list = get_node<godot::ItemList>("%SaveList");
  const auto selection = list->get_selected_items();
  list->clear();
  for (const auto& entry : entries_) {
    const auto name = entry.name.is_empty() ? tr("SAVE_LEGACY") : entry.name;
    const auto text = name + godot::String("\n") +
                      (entry.available ? entry.summary : tr("SAVE_DAMAGED")) +
                      "  |  " + entry.date;
    const auto index = list->add_item(text);
    list->set_item_metadata(index, entry.id);
    // Damaged saves can still be selected for deletion.
  }
  get_node<godot::Button>("%ConfirmButton")->set_disabled(true);
  get_node<godot::Button>("%DeleteButton")->set_disabled(true);
  if (!selection.is_empty() && selection[0] < list->get_item_count()) {
    list->select(selection[0]);
    get_node<godot::Button>("%ConfirmButton")
        ->set_disabled(!entries_[selection[0]].available);
    get_node<godot::Button>("%DeleteButton")->set_disabled(false);
  }
}

void LoadDialog::Select(int64_t index) {
  ERR_FAIL_INDEX(index, entries_.size());
  if (!delete_id_.is_empty()) {
    return;
  }
  get_node<godot::Button>("%DeleteButton")->set_disabled(false);
  get_node<godot::Button>("%ConfirmButton")
      ->set_disabled(!entries_[index].available);
  SetStatus("");
}

void LoadDialog::Activate(int64_t index) {
  ERR_FAIL_INDEX(index, entries_.size());
  if (delete_id_.is_empty() && entries_[index].available) {
    emit_signal("load_requested", entries_[index].id);
  }
}

void LoadDialog::Confirm() {
  const auto selection =
      get_node<godot::ItemList>("%SaveList")->get_selected_items();
  if (!selection.is_empty()) {
    Activate(selection[0]);
  }
}

void LoadDialog::RequestDelete() {
  const auto selection =
      get_node<godot::ItemList>("%SaveList")->get_selected_items();
  if (selection.is_empty() || !delete_id_.is_empty()) {
    return;
  }
  const auto& entry = entries_[selection[0]];
  delete_id_ = entry.id;
  delete_entry_ = entry;
  SetStatus("");
  RefreshDelete();
  get_node<godot::Button>("%CancelDeleteButton")->grab_focus();
}

void LoadDialog::ConfirmDelete() {
  if (delete_id_.is_empty()) {
    return;
  }
  const auto id = delete_id_;
  CancelDelete();
  emit_signal("delete_requested", id);
}

void LoadDialog::CancelDelete() {
  delete_id_ = "";
  delete_entry_ = Entry{};
  RefreshDelete();
  get_node<godot::ItemList>("%SaveList")->grab_focus();
}

void LoadDialog::RefreshDelete() {
  const bool pending = !delete_id_.is_empty();
  get_node<godot::Control>("%DeleteOverlay")->set_visible(pending);
  auto* list = get_node<godot::ItemList>("%SaveList");
  list->set_focus_mode(pending ? godot::Control::FOCUS_NONE
                               : godot::Control::FOCUS_ALL);
  const auto selection = list->get_selected_items();
  const bool selected = !selection.is_empty();
  get_node<godot::Button>("%ConfirmButton")
      ->set_disabled(pending || !selected || !entries_[selection[0]].available);
  get_node<godot::Button>("%DeleteButton")->set_disabled(pending || !selected);
  get_node<godot::Button>("%CancelButton")->set_disabled(pending);
  auto* label = get_node<godot::Label>("%DeleteLabel");
  label->set_text(
      (delete_entry_.name.is_empty() ? tr("SAVE_LEGACY") : delete_entry_.name) +
      godot::String("\n") +
      (delete_entry_.available ? delete_entry_.summary : tr("SAVE_DAMAGED")) +
      "  |  " + delete_entry_.date);
}

void LoadDialog::Cancel() {
  if (!delete_id_.is_empty()) {
    CancelDelete();
    return;
  }
  hide();
  emit_signal("cancelled");
}

void LoadDialog::_input(const godot::Ref<godot::InputEvent>& event) {
  if (!godot::Engine::get_singleton()->is_editor_hint() &&
      is_visible_in_tree() && event->is_action_pressed("ui_cancel")) {
    get_viewport()->set_input_as_handled();
    Cancel();
  }
}

void LoadDialog::SetStatus(godot::String key) {
  status_key_ = key;
  auto* label = get_node<godot::Label>("%StatusLabel");
  label->set_text(key.is_empty() ? godot::String() : tr(key));
  label->set_visible(!key.is_empty());
}

void LoadDialog::_notification(int what) {
  if (what == NOTIFICATION_TRANSLATION_CHANGED && is_node_ready() &&
      !godot::Engine::get_singleton()->is_editor_hint()) {
    RefreshEntries();
    RefreshDelete();
    SetStatus(status_key_);
  }
}
}  // namespace godotium
