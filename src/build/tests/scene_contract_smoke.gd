extends SceneTree


func _initialize() -> void:
	_run.call_deferred()


func _instance(folder: String) -> Node:
	var packed: PackedScene = load("res://godotium/scenes/" + folder + "/" + folder + ".tscn")
	var scene := packed.instantiate()
	assert(scene.get_script() == null)
	root.add_child(scene)
	return scene


func _escape() -> void:
	var event := InputEventKey.new()
	event.keycode = KEY_ESCAPE
	event.pressed = true
	root.push_input(event, true)
	event = event.duplicate()
	event.pressed = false
	root.push_input(event, true)


func _run() -> void:
	create_timer(12.0).timeout.connect(func(): quit(1))
	assert(root.get_node_or_null("MainScene") == null)
	var menu := _instance("main_menu")
	var actions: Array[String] = []
	menu.connect("new_game_requested", func(): actions.append("new"))
	menu.connect("quit_requested", func(): actions.append("quit"))
	menu.get_node("%NewGameButton").emit_signal("pressed")
	menu.get_node("%QuitButton").emit_signal("pressed")
	assert(actions == ["new", "quit"], "Menu requests actions without starting or quitting a game")
	menu.free()

	var first := _instance("save_dialog")
	var second := _instance("save_dialog")
	second.hide()
	var first_requests: Array = []
	var second_requests: Array = []
	first.connect("save_requested", func(name, overwrite): first_requests.append([name, overwrite]))
	second.connect("save_requested", func(name, overwrite): second_requests.append([name, overwrite]))
	var name: LineEdit = first.get_node("%SaveName")
	assert(name != second.get_node("%SaveName"), "Unique names must stay within their owning scene")
	assert(first.get_node("%ConfirmButton").disabled)
	name.text = "  Standalone / save  "
	name.emit_signal("text_changed", name.text)
	first.get_node("%ConfirmButton").emit_signal("pressed")
	assert(first_requests == [["Standalone / save", false]] and second_requests.is_empty())
	assert(first.visible, "Only the coordinator decides whether storage succeeded")
	var cancelled := [false]
	first.connect("cancelled", func(): cancelled[0] = true)
	name.grab_focus()
	_escape()
	assert(cancelled[0] and not first.visible, "Esc must cancel even with the name field focused")
	first.free()
	second.free()

	var load_dialog := _instance("load_dialog")
	var load_cancelled := [false]
	load_dialog.connect("cancelled", func(): load_cancelled[0] = true)
	assert(load_dialog.get_node("%SaveList").item_count == 0)
	assert(load_dialog.get_node("%ConfirmButton").disabled)
	_escape()
	assert(load_cancelled[0] and not load_dialog.visible)
	load_dialog.free()
	var save_directory := OS.get_environment("GODOTIUM_SAVE_DIR")
	assert(not save_directory.is_empty())
	assert(not DirAccess.dir_exists_absolute(save_directory), "Standalone forms must not write save files")

	var game := _instance("game_scene")
	var timer: Label = game.get_node("%TimerLabel")
	assert(timer.text == "00:00:00")
	await create_timer(1.1).timeout
	assert(timer.text != "00:00:00", "GameScene must run without MainScene")
	var paused := [false]
	game.connect("pause_requested", func(): paused[0] = true)
	_escape()
	assert(paused[0])
	var paused_time := timer.text
	await create_timer(1.1).timeout
	assert(timer.text == paused_time)
	game.free()
	print("PASS: independent native scenes, isolated node ownership and signal-only form actions")
	quit()
