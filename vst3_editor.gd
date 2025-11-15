extends Node2D
class_name Vst3Editor

var vst3: Vst3Instance
var slider_scene
var dropdown_scene
var container: VBoxContainer


func _ready():
	slider_scene = preload("res://vst3_control_slider.tscn")
	dropdown_scene = preload("res://vst3_control_dropdown.tscn")
	container = $ScrollContainer/VBoxContainer


func initialize(vst3_instance: Vst3Instance):
	vst3 = vst3_instance
	for input_parameter in vst3_instance.get_input_parameters():
		var value = vst3_instance.get_input_parameter_channel(input_parameter.index)

		if false:
			var dropdown: Vst3ControlDropdown = dropdown_scene.instantiate()
			dropdown.call_deferred("initialize", input_parameter, value)
			dropdown.value_changed.connect(_on_value_changed)
			container.add_child(dropdown)
		else:
			var slider: Vst3ControlSlider = slider_scene.instantiate()
			slider.call_deferred("initialize", input_parameter, value)
			slider.value_changed.connect(_on_value_changed)
			container.add_child(slider)


func update(vst3_instance: Vst3Instance):
	vst3 = vst3_instance
	for input_parameter in vst3_instance.get_input_parameters():
		var value = vst3_instance.get_input_parameter_channel(input_parameter.index)
		if input_parameter.get_choices().size() > 0:
			var dropdown: Vst3ControlDropdown = container.get_child(input_parameter.index)
			dropdown.dropdown.selected = int(value)
		else:
			var slider: Vst3ControlSlider = container.get_child(input_parameter.index)
			slider.line_edit.text = slider._format_value(value)


func _on_value_changed(control: int, value: float):
	vst3.send_input_parameter_channel(control, value)
