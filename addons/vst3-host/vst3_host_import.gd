@tool
extends EditorPlugin

var vst3_plugin: EditorVst3Instances


func _enter_tree():
	if DisplayServer.get_name() != "headless":
		vst3_plugin = (
			preload("res://addons/vst3-host/editor_vst3_host_instances.tscn").instantiate()
		)
		vst3_plugin.editor_interface = get_editor_interface()
		vst3_plugin.undo_redo = get_undo_redo()

		add_control_to_bottom_panel(vst3_plugin, "Vst3 Plugins")


func _exit_tree():
	if DisplayServer.get_name() != "headless":
		remove_control_from_bottom_panel(vst3_plugin)

	if vst3_plugin:
		vst3_plugin.queue_free()


func _notification(what):
	pass


func _has_main_screen():
	return false


func _make_visible(visible):
	pass


func _get_plugin_name():
	return "Vst3"


func _get_plugin_icon():
	# Must return some kind of Texture for the icon.
	return EditorInterface.get_editor_theme().get_icon("Node", "EditorIcons")
