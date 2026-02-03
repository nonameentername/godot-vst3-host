@tool
extends VBoxContainer
class_name EditorVst3Instances

var editor_interface: EditorInterface
var undo_redo: EditorUndoRedoManager
var edited_path: String
var save_timer: Timer
var container: HBoxContainer
var editor_packed_scene: PackedScene
var file_label: Label
var file_dialog: EditorFileDialog
var new_layout: bool
var renaming_instance: bool
var drop_end: EditorVst3Drop


func _init():
	renaming_instance = false

	file_dialog = EditorFileDialog.new()

	var extensions = ResourceLoader.get_recognized_extensions_for_type("Vst3Layout")
	for extension in extensions:
		file_dialog.add_filter("*.%s" % extension, "Vst3 Layout")

	add_child(file_dialog)
	file_dialog.connect("file_selected", _file_dialog_callback)

	Vst3Server.connect("layout_changed", _update)


func _ready():
	var layout = ProjectSettings.get_setting_with_override("audio/vst3-host/default_vst3_layout")
	if layout:
		edited_path = layout
	else:
		edited_path = "res://default_vst3_layout.tres"
	editor_packed_scene = preload("res://addons/vst3-host/editor_vst3_host_instance.tscn")

	save_timer = $SaveTimer
	container = $Vst3Scroll/Vst3HBox
	file_label = $TopBoxContainer/FileLabel

	_update()


func _process(_delta):
	pass


func _notification(what):
	match what:
		NOTIFICATION_ENTER_TREE:
			_update_theme()

		NOTIFICATION_THEME_CHANGED:
			_update_theme()

		NOTIFICATION_DRAG_END:
			if drop_end:
				container.remove_child(drop_end)
				drop_end.queue_free()
				drop_end = null

		NOTIFICATION_PROCESS:
			var edited = Vst3Server.get_edited()
			Vst3Server.set_edited(false)

			if edited:
				save_timer.start()


func _update_theme():
	var stylebox: StyleBoxEmpty = get_theme_stylebox("panel", "Tree")
	$Vst3Scroll.add_theme_stylebox_override("panel", stylebox)

	for control in $Vst3Scroll/Vst3HBox.get_children():
		control.editor_interface = editor_interface


func _add_instance():
	undo_redo.create_action("Add Vst3 Plugin")
	undo_redo.add_do_method(Vst3Server, "set_instance_count", Vst3Server.get_instance_count() + 1)
	undo_redo.add_undo_method(Vst3Server, "set_instance_count", Vst3Server.get_instance_count())
	undo_redo.add_do_method(self, "_update")
	undo_redo.add_undo_method(self, "_update")
	undo_redo.commit_action()


func _update():
	if renaming_instance:
		return

	for i in range(container.get_child_count(), 0, -1):
		i = i - 1
		var editor_instance: Control = container.get_child(i)
		if editor_instance:
			container.remove_child(editor_instance)
			editor_instance.queue_free()

	if drop_end:
		container.remove_child(drop_end)
		drop_end.queue_free()
		drop_end = null

	for i in range(0, Vst3Server.get_instance_count()):
		var editor_instance: EditorVst3Instance = editor_packed_scene.instantiate()
		editor_instance.editor_interface = editor_interface
		var is_main: bool = i == 0
		editor_instance.editor_instances = self
		editor_instance.is_main = is_main
		editor_instance.undo_redo = undo_redo
		container.add_child(editor_instance)

		editor_instance.connect(
			"delete_request", _delete_instance.bind(editor_instance), CONNECT_DEFERRED
		)
		editor_instance.connect("duplicate_request", _duplicate_instance, CONNECT_DEFERRED)
		editor_instance.connect(
			"vol_reset_request", _reset_volume.bind(editor_instance), CONNECT_DEFERRED
		)
		editor_instance.connect("drop_end_request", _request_drop_end, CONNECT_DEFERRED)
		editor_instance.connect("dropped", _drop_at_index, CONNECT_DEFERRED)


func _update_instance(index):
	if index >= container.get_child_count():
		return

	container.get_child(index).update_instance()


func _delete_instance(editor_instance: EditorVst3Instance):
	var index: int = editor_instance.get_index()
	if index == 0:
		push_warning("Main Vst3 can't be deleted!")
		return

	undo_redo.create_action("Delete Vst3 Plugin")
	undo_redo.add_do_method(Vst3Server, "remove_instance", index)
	undo_redo.add_undo_method(Vst3Server, "add_instance", index)
	undo_redo.add_undo_method(
		Vst3Server, "set_instance_name", index, Vst3Server.get_instance_name(index)
	)
	undo_redo.add_undo_method(Vst3Server, "set_volume_db", index, Vst3Server.get_volume_db(index))
	undo_redo.add_undo_method(Vst3Server, "set_solo", index, Vst3Server.is_solo(index))
	undo_redo.add_undo_method(Vst3Server, "set_mute", index, Vst3Server.is_mute(index))
	undo_redo.add_undo_method(Vst3Server, "set_bypass", index, Vst3Server.is_bypassing(index))

	undo_redo.add_do_method(self, "_update")
	undo_redo.add_undo_method(self, "_update")
	undo_redo.commit_action()


func _duplicate_instance(which: int):
	var add_at_pos: int = which + 1
	undo_redo.create_action("Duplicate Vst3 Plugin")
	undo_redo.add_do_method(Vst3Server, "add_instance", add_at_pos)
	undo_redo.add_do_method(
		Vst3Server, "set_instance_name", add_at_pos, Vst3Server.get_instance_name(which) + " Copy"
	)
	undo_redo.add_do_method(
		Vst3Server, "set_volume_db", add_at_pos, Vst3Server.get_volume_db(which)
	)
	undo_redo.add_do_method(Vst3Server, "set_solo", add_at_pos, Vst3Server.is_solo(which))
	undo_redo.add_do_method(Vst3Server, "set_mute", add_at_pos, Vst3Server.is_mute(which))
	undo_redo.add_do_method(Vst3Server, "set_bypass", add_at_pos, Vst3Server.is_bypassing(which))
	undo_redo.add_do_method(Vst3Server, "set_uri", add_at_pos, Vst3Server.get_uri(which))
	undo_redo.add_undo_method(Vst3Server, "remove_instance", add_at_pos)
	undo_redo.add_do_method(self, "_update")
	undo_redo.add_undo_method(self, "_update")
	undo_redo.commit_action()


func _reset_volume(editor_instance: EditorVst3Instance):
	var index: int = editor_instance.get_index()
	undo_redo.create_action("Reset Vst3 Volume")
	undo_redo.add_do_method(Vst3Server, "set_volume_db", index, 0)
	undo_redo.add_undo_method(Vst3Server, "set_volume_db", index, Vst3Server.get_volume_db(index))
	undo_redo.add_do_method(self, "_update")
	undo_redo.add_undo_method(self, "_update")
	undo_redo.commit_action()


func _request_drop_end():
	if not drop_end and container.get_child_count():
		drop_end = EditorVst3Drop.new()
		container.add_child(drop_end)
		drop_end.custom_minimum_size = container.get_child(0).size
		drop_end.connect("dropped", _drop_at_index, CONNECT_DEFERRED)


func _drop_at_index(instance, index):
	undo_redo.create_action("Move Vst3")

	undo_redo.add_do_method(Vst3Server, "move_instance", instance, index)
	var real_instance: int = instance if index > instance else instance + 1
	var real_index: int = index - 1 if index > instance else index
	undo_redo.add_undo_method(Vst3Server, "move_instance", real_index, real_instance)

	undo_redo.add_do_method(self, "_update")
	undo_redo.add_undo_method(self, "_update")
	undo_redo.commit_action()


func _server_save():
	var status = Vst3Server.generate_layout()
	ResourceSaver.save(status, edited_path)


func _save_as_layout():
	file_dialog.file_mode = EditorFileDialog.FILE_MODE_SAVE_FILE
	file_dialog.title = "Save Vst3 Layout As..."
	file_dialog.current_path = edited_path
	file_dialog.popup_centered_clamped(Vector2(1050, 700) * scale, 0.8)
	new_layout = false


func _new_layout():
	file_dialog.file_mode = EditorFileDialog.FILE_MODE_SAVE_FILE
	file_dialog.title = "Location for New Layout..."
	file_dialog.current_path = edited_path
	file_dialog.popup_centered_clamped(Vector2(1050, 700) * scale, 0.8)
	new_layout = false


func _select_layout():
	if editor_interface:
		editor_interface.get_file_system_dock().navigate_to_path(edited_path)


func _load_layout():
	file_dialog.file_mode = EditorFileDialog.FILE_MODE_OPEN_FILE
	file_dialog.title = "Open Vst3 Layout"
	file_dialog.current_path = edited_path
	file_dialog.popup_centered_clamped(Vector2(1050, 700) * scale, 0.8)
	new_layout = false


func _load_default_layout():
	var layout_path = ProjectSettings.get_setting_with_override(
		"audio/vst3-host/default_vst3_layout"
	)
	var state = ResourceLoader.load(layout_path, "", ResourceLoader.CACHE_MODE_IGNORE)
	if not state:
		push_warning("There is no '%s' file." % layout_path)
		return

	edited_path = layout_path
	file_label.text = "Layout: %s" % edited_path.get_file()
	Vst3Server.set_layout(state)
	_update()
	call_deferred("_select_layout")


func _file_dialog_callback(filename: String):
	if file_dialog.file_mode == EditorFileDialog.FILE_MODE_OPEN_FILE:
		var state = ResourceLoader.load(filename, "", ResourceLoader.CACHE_MODE_IGNORE)
		if not state:
			push_warning("Invalid file, not a vst3 layout.")
			return

		edited_path = filename
		file_label.text = "Layout: %s" % edited_path.get_file()
		Vst3Server.set_layout(state)
		_update()
		call_deferred("_select_layout")

	elif file_dialog.file_mode == EditorFileDialog.FILE_MODE_SAVE_FILE:
		if new_layout:
			var vst3_layout: Vst3Layout = Vst3Layout.new()
			Vst3Server.set_layout(vst3_layout)

		var error = ResourceSaver.save(Vst3Server.generate_layout(), filename)
		if error != OK:
			push_warning("Error saving file: %s" % filename)
			return

		edited_path = filename
		file_label.text = "Layout: %s" % edited_path.get_file()
		_update()
		call_deferred("_select_layout")


func _set_renaming(renaming: bool):
	renaming_instance = renaming


func resource_saved(resource: Resource):
	for editor_instance in container.get_children():
		if editor_instance is EditorVst3Instance:
			editor_instance.resource_saved(resource)
