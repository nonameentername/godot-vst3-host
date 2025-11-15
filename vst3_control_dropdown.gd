extends HBoxContainer
class_name Vst3ControlDropdown

signal value_changed(control_index: int, value: float)

@onready var label: Label = $Label
@onready var dropdown: OptionButton = $OptionButton

var control_index: int
var integer: bool


func initialize(vst3_control: Vst3Parameter, _value: float) -> void:
	control_index = vst3_control.index
	integer = vst3_control.integer

	label.text = vst3_control.name

	var choices = vst3_control.get_choices()
	for choice in choices:
		dropdown.add_item(choice, choices[choice])


func _on_option_button_item_selected(index: int) -> void:
	var value = dropdown.get_item_id(index)
	emit_signal("value_changed", control_index, float(value))
