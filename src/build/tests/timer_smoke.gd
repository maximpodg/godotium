extends SceneTree

var main: Node
var panel: Control
var confirm: Button
var status: Label


func _initialize() -> void:
	_run.call_deferred()


func _escape() -> void:
	var event := InputEventKey.new()
	event.keycode = KEY_ESCAPE
	event.pressed = true
	root.push_input(event, true)
	event = event.duplicate()
	event.pressed = false
	root.push_input(event, true)


func _press(node_name: String) -> void:
	if node_name == "SaveMenuConfirm":
		confirm.emit_signal("pressed")
		return
	main.get_node("%MainMenu").get_node("%" + node_name).emit_signal("pressed")
	if node_name == "SaveGameButton" or node_name == "LoadGameButton":
		panel = main.get_node("%SaveDialog" if node_name == "SaveGameButton" else "%LoadDialog")
		confirm = panel.get_node("%ConfirmButton")
		status = panel.get_node("%StatusLabel")


func _name(value: String) -> void:
	var edit: LineEdit = main.get_node("%SaveDialog").get_node("%SaveName")
	edit.text = value
	edit.emit_signal("text_changed", value)


func _select_path(path: String) -> int:
	var list: ItemList = main.get_node("%LoadDialog").get_node("%SaveList")
	for index in list.item_count:
		if list.get_item_metadata(index) == path:
			list.select(index)
			list.emit_signal("item_selected", index)
			return index
	assert(false, "Save missing from list: " + path)
	return -1


func _run() -> void:
	create_timer(20.0).timeout.connect(func(): quit(1))
	var directory := OS.get_environment("GODOTIUM_SAVE_DIR")
	assert(not directory.is_empty(), "Tests need an isolated save directory")
	var first_name := "Первый / день"
	var first_path := directory.path_join(first_name.to_lower().sha256_text() + ".cfg")
	var second_name := "Second game"
	var second_path := directory.path_join(second_name.to_lower().sha256_text() + ".cfg")
	var scene: PackedScene = load(ProjectSettings.get_setting("application/run/main_scene"))
	main = scene.instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	var menu: Control = main.get_node("%MainMenu")
	var game: Control = main.get_node("%GameScene")
	var timer: Label = main.get_node("%GameScene").get_node("%TimerLabel")
	var list: ItemList = main.get_node("%LoadDialog").get_node("%SaveList")
	if "--reload" in OS.get_cmdline_user_args():
		_press("LoadGameButton")
		assert(list.item_count == 2)
		var index := _select_path(second_path)
		assert(list.get_item_text(index).contains(second_name))
		assert(list.get_item_text(index).contains("01:01:01"))
		assert(list.get_item_text(index).contains("UTC"))
		list.emit_signal("item_activated", index)
		assert(game.visible and timer.text == "01:01:01")
		await create_timer(1.1).timeout
		assert(timer.text != "01:01:01")
		_escape()
		# Damaged saves remain deletable; cancel and Esc preserve the file.
		var damaged := ConfigFile.new()
		damaged.set_value("game", "version", 99)
		assert(damaged.save(first_path) == OK)
		_press("LoadGameButton")
		assert(panel.get_node("%DeleteButton").disabled)
		_select_path(first_path)
		assert(confirm.disabled)
		panel.get_node("%DeleteButton").emit_signal("pressed")
		assert(panel.get_node("%DeleteOverlay").visible)
		assert(list.visible and panel.get_node("%Actions").visible)
		assert(confirm.disabled and panel.get_node("%CancelButton").disabled)
		assert(panel.get_node("%CancelDeleteButton").has_focus())
		_escape()
		assert(panel.visible and list.visible and FileAccess.file_exists(first_path))
		panel.get_node("%DeleteButton").emit_signal("pressed")
		panel.get_node("%CancelDeleteButton").emit_signal("pressed")
		assert(FileAccess.file_exists(first_path))
		# A file removed externally reports an error without deleting another slot.
		panel.get_node("%DeleteButton").emit_signal("pressed")
		assert(DirAccess.remove_absolute(first_path) == OK)
		panel.get_node("%ConfirmDeleteButton").emit_signal("pressed")
		assert(status.text == main.tr("SAVE_DELETE_FAILED"))
		assert(FileAccess.file_exists(second_path))
		assert(damaged.save(first_path) == OK)
		panel.get_node("%DeleteButton").emit_signal("pressed")
		panel.get_node("%ConfirmDeleteButton").emit_signal("pressed")
		assert(not FileAccess.file_exists(first_path) and list.item_count == 1)
		assert(panel.visible and not game.visible)
		_select_path(second_path)
		panel.get_node("%DeleteButton").emit_signal("pressed")
		assert(panel.get_node("%DeleteLabel").text.contains(second_name))
		assert(panel.get_node("%DeleteLabel").text.contains("01:01:01"))
		assert(panel.get_node("%DeleteLabel").text.contains("UTC"))
		panel.get_node("%ConfirmDeleteButton").emit_signal("pressed")
		assert(not FileAccess.file_exists(second_path) and list.item_count == 0)
		assert(confirm.disabled and panel.get_node("%DeleteButton").disabled)
		assert(status.text == main.tr("SAVE_EMPTY"))
		_escape()
		_escape()
		assert(game.visible, "Deleting files must preserve the running game")
		print("PASS: fresh-process loading; delete/cancel, damaged saves, errors and empty list")
		quit()
		return
	assert(menu.visible and not game.visible)
	assert(not main.get_node("%MainMenu").get_node("%SaveGameButton").visible)
	_press("LoadGameButton")
	assert(panel.visible and list.item_count == 0 and confirm.disabled)
	assert(status.text == main.tr("SAVE_EMPTY"), "Actual: " + status.text + "; expected: " + main.tr("SAVE_EMPTY"))
	_escape()
	assert(menu.visible and not panel.visible)
	_press("NewGameButton")
	assert(game.visible and timer.text == "00:00:00")
	await create_timer(1.1).timeout
	_escape()
	var paused_time := timer.text
	assert(paused_time != "00:00:00")
	_press("SaveGameButton")
	assert(panel.visible and confirm.disabled)
	assert(panel.get_node("%SaveList").item_count == 0)
	_name("   ")
	assert(confirm.disabled)
	_name("  " + first_name + "  ")
	assert(not confirm.disabled)
	await create_timer(1.1).timeout
	assert(timer.text == paused_time)
	_press("SaveMenuConfirm")
	assert(menu.visible and not panel.visible)
	var saved := ConfigFile.new()
	assert(saved.load(first_path) == OK)
	var seconds: float = saved.get_value("game", "elapsed_seconds")
	assert(seconds >= 1.0 and seconds < 2.5)
	assert(saved.get_value("game", "name") == first_name)
	assert(saved.get_value("game", "saved_at") > 0)
	# Settings and save cancellation return to the paused menu.
	_press("SettingsButton")
	_escape()
	assert(menu.visible and not game.visible)
	_press("SaveGameButton")
	_escape()
	assert(menu.visible and not game.visible)
	_escape()
	await create_timer(1.1).timeout
	assert(game.visible and timer.text != paused_time)
	_escape()
	# A second name creates another file, preserving the first.
	_press("SaveGameButton")
	assert(main.get_node("%SaveDialog").get_node("%SaveName").text == first_name)
	_name(second_name)
	_press("SaveMenuConfirm")
	assert(saved.load(first_path) == OK and saved.get_value("game", "elapsed_seconds") == seconds)
	assert(FileAccess.file_exists(second_path))
	# Existing names disable Save; Enter defaults to Cancel, never Overwrite.
	_press("SaveGameButton")
	assert(panel.get_node("%CancelButton").has_focus())
	_name("  " + first_name.to_upper() + "  ")
	assert(confirm.disabled and not panel.get_node("%OverwriteButton").disabled)
	_press("SaveMenuConfirm")
	assert(panel.visible)
	panel.get_node("%SaveName").emit_signal("text_submitted", first_name)
	assert(menu.visible and not panel.visible)
	assert(saved.load(first_path) == OK and saved.get_value("game", "elapsed_seconds") == seconds)
	_press("SaveGameButton")
	var save_list: ItemList = panel.get_node("%SaveList")
	assert(save_list.item_count == 2)
	for index in save_list.item_count:
		if save_list.get_item_text(index).begins_with(first_name + "\n"):
			save_list.select(index)
			save_list.emit_signal("item_selected", index)
	assert(panel.get_node("%SaveName").text == first_name)
	assert(confirm.disabled and panel.get_node("%CancelButton").has_focus())
	assert(not panel.get_node("%OverwriteButton").disabled)
	panel.get_node("%OverwriteButton").emit_signal("pressed")
	assert(not panel.visible, "Overwrite must save immediately without another prompt")
	assert(saved.load(first_path) == OK and saved.get_value("game", "elapsed_seconds") > seconds)
	_press("SaveGameButton")
	_name("Different")
	assert(not confirm.disabled and confirm.has_focus())
	assert(panel.get_node("%OverwriteButton").disabled)
	_name("  ")
	assert(confirm.disabled and panel.get_node("%OverwriteButton").disabled)
	assert(panel.get_node("%CancelButton").has_focus())
	_escape()
	# A slot created externally while the form is open must not be overwritten by Save.
	_press("SaveGameButton")
	_name("External")
	var external_path := directory.path_join("external".sha256_text() + ".cfg")
	assert(saved.save(external_path) == OK)
	_press("SaveMenuConfirm")
	assert(panel.visible and confirm.disabled)
	assert(panel.get_node("%CancelButton").has_focus())
	assert(not panel.get_node("%OverwriteButton").disabled)
	_escape()
	assert(DirAccess.remove_absolute(external_path) == OK)
	# Loading selects a slot; a new game resets time without deleting any saves.
	_press("NewGameButton")
	assert(timer.text == "00:00:00")
	_escape()
	_press("LoadGameButton")
	assert(list.item_count == 2 and confirm.disabled)
	_select_path(second_path)
	_press("SaveMenuConfirm")
	assert(game.visible and timer.text != "00:00:00")
	_escape()
	# Failed writes keep the form open, with the entered name and old files intact.
	OS.set_environment("GODOTIUM_SAVE_DIR", first_path)
	_press("SaveGameButton")
	_name("Failure")
	_press("SaveMenuConfirm")
	assert(panel.visible and status.text == main.tr("GAME_SAVE_FAILED"))
	assert(main.get_node("%SaveDialog").get_node("%SaveName").text == "Failure")
	OS.set_environment("GODOTIUM_SAVE_DIR", directory)
	_escape()
	# Invalid entries remain visible but cannot be selected for loading.
	assert(saved.load(second_path) == OK)
	saved.set_value("game", "elapsed_seconds", -1.0)
	assert(saved.save(second_path) == OK)
	_press("LoadGameButton")
	for index in list.item_count:
		if list.get_item_metadata(index) == second_path:
			assert(not list.is_item_disabled(index))
			_select_path(second_path)
			assert(confirm.disabled)
			assert(not panel.get_node("%DeleteButton").disabled)
			assert(list.get_item_text(index).contains(main.tr("SAVE_DAMAGED")))
	_escape()
	saved.set_value("game", "elapsed_seconds", 3661.25)
	assert(saved.save(second_path) == OK)
	_press("LoadGameButton")
	_select_path(second_path)
	# Revalidate the file when loading, in case it changed while the list was open.
	saved.set_value("game", "version", 99)
	assert(saved.save(second_path) == OK)
	_press("SaveMenuConfirm")
	assert(panel.visible and status.text == main.tr("GAME_LOAD_FAILED"))
	saved.set_value("game", "version", 1)
	assert(saved.save(second_path) == OK)
	_press("SaveMenuConfirm")
	assert(game.visible and timer.text == "01:01:01")
	_escape()
	# The old unnamed slot remains loadable without being overwritten.
	var legacy := directory.path_join("../savegame.cfg").simplify_path()
	var old := ConfigFile.new()
	old.set_value("game", "version", 1)
	old.set_value("game", "elapsed_seconds", 42.0)
	assert(old.save(legacy) == OK)
	OS.set_environment("GODOTIUM_SAVE_PATH", legacy)
	_press("LoadGameButton")
	assert(list.item_count == 3)
	_select_path(legacy)
	_press("SaveMenuConfirm")
	assert(timer.text == "00:00:42")
	_escape()
	_press("LoadGameButton")
	_select_path(legacy)
	panel.get_node("%DeleteButton").emit_signal("pressed")
	panel.get_node("%ConfirmDeleteButton").emit_signal("pressed")
	assert(not FileAccess.file_exists(legacy) and list.item_count == 2)
	OS.unset_environment("GODOTIUM_SAVE_PATH")
	print("PASS: named slots, pause, save/load, overwrite/cancel, errors and legacy save")
	quit()
