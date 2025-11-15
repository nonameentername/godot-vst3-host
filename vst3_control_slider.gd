extends HBoxContainer
class_name Vst3ControlSlider

signal value_changed(control_index: int, value: float)

@onready var label: Label = $Label
@onready var slider: HSlider = $HSlider
@onready var line_edit: LineEdit = $LineEdit

var control_index: int
var min_value: float
var max_value: float
var integer: bool
var decimals: int = 2

var _updating := false

const epsilon := 0.0001


func initialize(vst3_parameter: Vst3Parameter, value: float) -> void:
	control_index = vst3_parameter.index
	min_value = 0
	max_value = 1
	integer = false

	label.text = vst3_parameter.title

	slider.min_value = 0.0
	slider.max_value = 1.0
	slider.step = 0.001

	var default: float = clamp(value, min_value, max_value)
	_update_ui_from_parameter(default, false)


func _normalized_to_log(normalized_value: float) -> float:
	if max_value <= 0.0:
		return 0.0

	if min_value == max_value:
		return min_value

	if min_value <= 0.0:
		if normalized_value <= 0.0:
			return 0.0
		else:
			return epsilon * pow(max_value / epsilon, normalized_value)
	else:
		return min_value * pow(max_value / min_value, normalized_value)


func _log_to_norm(value: float) -> float:
	if max_value <= 0.0:
		return 0.0

	if min_value == max_value:
		return 0.0

	if min_value <= 0.0:
		if value <= 0.0:
			return 0.0
		else:
			return log(value / epsilon) / log(max_value / epsilon)
	else:
		return log(value / min_value) / log(max_value / min_value)


func _format_value(value: float) -> String:
	if integer:
		value = round(value)

	return str(snapped(value, pow(10.0, -decimals)))


func _clamp_and_quantize(value: float) -> float:
	var low_value = min_value

	if min_value <= 0.0:
		low_value = 0.0

	value = clamp(value, low_value, max_value)

	if integer:
		value = round(value)

	return value


func _on_h_slider_value_changed(sv: float) -> void:
	if _updating:
		return

	_updating = true
	var param_value: float

	param_value = sv

	param_value = _clamp_and_quantize(param_value)
	line_edit.text = _format_value(param_value)

	emit_signal("value_changed", control_index, param_value)

	_updating = false


func _apply_text(text: String) -> void:
	if _updating:
		return

	text = text.strip_edges()

	if text == "" or not text.is_valid_float():
		return

	_updating = true
	var value := _clamp_and_quantize(text.to_float())

	slider.value = value

	line_edit.text = _format_value(value)

	emit_signal("value_changed", control_index, value)

	_updating = false


func _update_ui_from_parameter(value: float, _emit_signal: bool = true) -> void:
	value = _clamp_and_quantize(value)
	_updating = true

	slider.value = value

	line_edit.text = _format_value(value)

	emit_signal("value_changed", control_index, value)

	_updating = false


func _on_line_edit_text_submitted(text: String) -> void:
	_apply_text(text)


func _on_line_edit_text_changed(text: String) -> void:
	_apply_text(text)


func _on_line_edit_focus_exited() -> void:
	_apply_text(line_edit.text)
