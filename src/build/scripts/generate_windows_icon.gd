extends SceneTree

# Run with: godot --headless --script generate_windows_icon.gd -- source.svg output.ico
func _initialize() -> void:
	var args := OS.get_cmdline_user_args()
	if args.size() != 2:
		push_error("Expected source.svg and output.ico")
		quit(1)
		return
	var source := Image.new()
	if source.load_svg_from_string(FileAccess.get_file_as_string(args[0])) != OK:
		quit(1)
		return
	var sizes := [16, 24, 32, 48, 64, 128, 256]
	var images: Array[PackedByteArray] = []
	for size in sizes:
		var scaled := Image.new()
		scaled.copy_from(source)
		scaled.resize(size, size, Image.INTERPOLATE_LANCZOS)
		images.append(scaled.save_png_to_buffer())
	var output := FileAccess.open(args[1], FileAccess.WRITE)
	if output == null:
		quit(1)
		return
	output.store_16(0)
	output.store_16(1)
	output.store_16(sizes.size())
	var offset := 6 + sizes.size() * 16
	for index in sizes.size():
		output.store_8(sizes[index] % 256)
		output.store_8(sizes[index] % 256)
		output.store_16(0)
		output.store_16(1)
		output.store_16(32)
		output.store_32(images[index].size())
		output.store_32(offset)
		offset += images[index].size()
	for data in images:
		output.store_buffer(data)
	output.close()
	quit()
