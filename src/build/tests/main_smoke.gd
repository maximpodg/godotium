extends SceneTree


func _initialize() -> void:
	_run.call_deferred()


func _fail(message: String) -> void:
	printerr(message)
	quit(1)


func _click(button: Button) -> void:
	var click := InputEventMouseButton.new()
	click.button_index = MOUSE_BUTTON_LEFT
	click.position = button.get_global_rect().get_center()
	click.pressed = true
	root.push_input(click, true)
	var release := click.duplicate()
	release.pressed = false
	root.push_input(release, true)


func _key(code: Key) -> void:
	var key := InputEventKey.new()
	key.keycode = code
	key.pressed = true
	root.push_input(key, true)
	var release := key.duplicate()
	release.pressed = false
	root.push_input(release, true)


func _run() -> void:
	var args := OS.get_cmdline_user_args()
	var source_project := "--source" in args
	if not source_project:
		if not FileAccess.file_exists("res://project.binary"):
			_fail("Expected packaged game resources, not the source project.")
			return
		for unwanted in ["res://build/export/BUILD.gn", "res://godotium/scenes/main_scene/main_scene.cc"]:
			if FileAccess.file_exists(unwanted):
				_fail("Build sources must not enter the resource pack.")
				return
	var second := "--mouse" in args
	var initial := "ru" if second else "en"
	var changed := "en" if second else "ru"
	# No main scene or settings panel exists yet. Autoload must already have
	# applied preferences, including those persisted by the preceding process.
	if root.get_node_or_null("Settings") == null or TranslationServer.get_locale() != initial:
		_fail("Settings and language must initialize before the startup scene.")
		return
	if not is_equal_approx(AudioServer.get_bus_volume_linear(AudioServer.get_bus_index("Music")), 0.37 if second else 0.30):
		_fail("Saved music volume must apply without creating a settings panel.")
		return
	if AudioServer.is_bus_mute(AudioServer.get_bus_index("SFX")) != second:
		_fail("Saved sound mute must apply without creating a settings panel.")
		return
	var scene: PackedScene = load(ProjectSettings.get_setting("application/run/main_scene"))
	var main := scene.instantiate()
	if not main.is_class("MainScene") or main.get_script() != null:
		_fail("Expected the native C++ MainScene class without a gameplay script.")
		return
	root.add_child(main)
	await process_frame
	await process_frame
	var menu: Control = main.get_node("%MainMenu")
	var exit: Button = menu.get_node("%QuitButton")
	var settings_button: Button = menu.get_node("%SettingsButton")
	var panel: PanelContainer = main.get_node("%SettingsScene")
	if panel.scene_file_path != "res://godotium/scenes/settings_scene/settings_scene.tscn":
		_fail("Settings must be an instance of its own scene.")
		return
	var music: AudioStreamPlayer = main.get_node("%MusicPlayer")
	if panel.visible or not menu.get_node("%NewGameButton").has_focus() or not music.playing or music.stream.get_length() < 10:
		_fail("Expected the menu with initial New Game focus and background music playing.")
		return
	var tracks: VBoxContainer = panel.get_node("%TrackRows")
	assert(tracks.get_child_count() == 2)
	var first_enabled: CheckButton = tracks.get_child(0).get_node("Enabled")
	var second_enabled: CheckButton = tracks.get_child(1).get_node("Enabled")
	if second:
		assert(not first_enabled.button_pressed, "Disabled tracks must persist across processes")
		assert(music.stream.resource_path.ends_with("/out_in_space.ogg"))
		first_enabled.button_pressed = true
	# Exercise the end-of-track connection, including wrapping the playlist.
	var first_track := music.stream.resource_path
	music.emit_signal("finished")
	if not music.playing or music.stream.resource_path == first_track or music.stream.get_length() < 10:
		_fail("The second music track did not start.")
		return
	music.emit_signal("finished")
	if music.stream.resource_path != first_track:
		_fail("The playlist did not wrap around.")
		return
	if TranslationServer.get_locale() != initial or settings_button.text != ("Настройки" if second else "Settings"):
		_fail("Initial language fallback/persistence or menu translation failed.")
		return
	if second:
		settings_button.grab_focus()
		_key(KEY_SPACE)
	else:
		_click(settings_button)
	await process_frame
	if not panel.visible or exit.is_visible_in_tree():
		_fail("Opening Settings must replace the main menu.")
		return
	var language: OptionButton = panel.get_node("%LanguageOption")
	if language.item_count != 2 or language.get_item_text(0) != "English" or language.get_item_text(1) != "Русский":
		_fail("Expected language choices using each language's own name.")
		return
	language.grab_focus()
	_key(KEY_SPACE)
	await process_frame
	_key(KEY_UP if second else KEY_DOWN)
	_key(KEY_ENTER)
	await process_frame
	await process_frame
	var tabs: TabContainer = panel.get_node("%SettingsTabs")
	if TranslationServer.get_locale() != changed or tabs.get_tab_title(0) != ("General" if second else "Основные"):
		_fail("Language selection must immediately translate the open settings panel.")
		return
	tabs.current_tab = 1
	await process_frame
	var first_play: Button = tracks.get_child(0).get_node("Play")
	var second_play: Button = tracks.get_child(1).get_node("Play")
	_click(first_play)
	assert(music.stream.resource_path.ends_with("/out_in_space_menu.ogg"))
	_click(second_play)
	assert(music.stream.resource_path.ends_with("/out_in_space.ogg"))
	second_enabled.button_pressed = false
	assert(second_play.disabled and music.stream.resource_path.ends_with("/out_in_space_menu.ogg"))
	first_enabled.button_pressed = false
	assert(not music.playing, "Disabling every track must stop music")
	second_enabled.button_pressed = true
	assert(music.playing and music.stream.resource_path.ends_with("/out_in_space.ogg"))
	music.emit_signal("finished")
	assert(music.stream.resource_path.ends_with("/out_in_space.ogg"), "Disabled tracks must be skipped")
	first_enabled.button_pressed = true
	# Six rows must scroll within a five-row viewport instead of growing the panel.
	var scroll: ScrollContainer = panel.get_node("%TrackScroll")
	for index in 4:
		var extra := Control.new()
		extra.custom_minimum_size.y = 36
		tracks.add_child(extra)
	await process_frame
	await process_frame
	assert(is_equal_approx(scroll.size.y, 180))
	assert(scroll.get_v_scroll_bar().max_value > scroll.get_v_scroll_bar().page)
	for index in range(5, 1, -1):
		tracks.get_child(index).queue_free()
	await process_frame
	var music_slider: HSlider = panel.get_node("%MusicVolume")
	var sound_slider: HSlider = panel.get_node("%SoundVolume")
	if music_slider.value != (37 if second else 30) or sound_slider.value != (0 if second else 70):
		_fail("Volume defaults/persistence failed.")
		return
	music_slider.value = 37
	sound_slider.value = 55
	_click(panel.get_node("%TestSound"))
	if not panel.get_node("%SoundPlayer").playing:
		_fail("Sound preview did not play.")
		return
	sound_slider.value = 0
	var music_bus := AudioServer.get_bus_index("Music")
	var sound_bus := AudioServer.get_bus_index("SFX")
	if not is_equal_approx(AudioServer.get_bus_volume_linear(music_bus), 0.37) or not AudioServer.is_bus_mute(sound_bus):
		_fail("Music and sound volume controls must affect separate audio buses; zero must mute.")
		return
	tabs.current_tab = 2
	await process_frame
	var fullscreen: CheckButton = panel.get_node("%Fullscreen")
	var vsync: CheckButton = panel.get_node("%Vsync")
	if fullscreen.button_pressed != second or vsync.button_pressed == second:
		_fail("Graphics defaults/persistence failed.")
		return
	# Fullscreen is exercised headlessly so tests never resize the user's desktop.
	if DisplayServer.get_name() == "headless":
		_click(fullscreen)
	_click(vsync)
	var saved := ConfigFile.new()
	if saved.load(OS.get_environment("GODOTIUM_SETTINGS_PATH")) != OK or saved.get_value("interface", "language") != changed or saved.get_value("audio", "music") != 37 or saved.get_value("audio", "sound") != 0 or saved.get_value("graphics", "vsync") != second:
		_fail("Changed preferences were not saved together.")
		return
	var capture_index := args.find("--capture")
	if capture_index >= 0:
		tabs.current_tab = 1
		await process_frame
		await RenderingServer.frame_post_draw
		if root.get_texture().get_image().save_png(args[capture_index + 1]) != OK:
			_fail("Could not save settings screenshot.")
			return
	if second:
		_click(panel.get_node("%CloseSettings"))
	else:
		_key(KEY_ESCAPE)
	await process_frame
	if panel.visible or not settings_button.has_focus():
		_fail("Back/Escape must return to the menu and restore keyboard focus.")
		return
	# The extracted panel must also work without MainScene or its named menu nodes.
	var standalone_scene: PackedScene = load("res://godotium/scenes/settings_scene/settings_scene.tscn")
	var standalone := standalone_scene.instantiate()
	var closed := [false]
	standalone.connect("closed", func(): closed[0] = true)
	root.add_child(standalone)
	await process_frame
	standalone.get_node("%MusicVolume").value = 61
	standalone.get_node("%CloseSettings").emit_signal("pressed")
	if standalone.visible or not closed[0]:
		_fail("Standalone SettingsScene must close and notify its owner.")
		return
	standalone.free()
	settings_button.emit_signal("pressed")
	if music_slider.value != 61 or panel.get_node("%MusicValue").text != "61%":
		_fail("Reopening SettingsScene must reflect preferences changed by another panel.")
		return
	# Keep the persisted values expected by the next process.
	music_slider.value = 37
	panel.get_node("%CloseSettings").emit_signal("pressed")
	first_enabled.button_pressed = second
	assert(saved.load(OS.get_environment("GODOTIUM_SETTINGS_PATH")) == OK)
	assert(saved.get_value("music_tracks", "out_in_space_menu") == second)
	create_timer(5.0).timeout.connect(func(): _fail("Exit button did not quit the game."))
	if second:
		_click(exit)
	else:
		exit.grab_focus()
		_key(KEY_SPACE)
