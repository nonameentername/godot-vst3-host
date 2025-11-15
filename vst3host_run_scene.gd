extends SceneTree

var plugin: Vst3Instance


func _init():
	var main_scene = preload("res://main.tscn")
	var main = main_scene.instantiate()

	Vst3Server.layout_changed.connect(on_layout_changed)
	Vst3Server.vst3_ready.connect(on_ready)

	change_scene_to_packed(main_scene)


func on_layout_changed():
	print("layout_changed")


func on_ready(_name):
	plugin = Vst3Server.get_instance("Main")

	plugin.note_on(0, 0, 64, 64)
	plugin.note_off(0, 0, 64)

	print("it works!")
