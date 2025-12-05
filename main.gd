extends Node2D

@onready var dropdown: OptionButton = $OptionButton
var editor: Vst3Editor
var vst3: Vst3Instance
var bus = 0
var channel = 0


func _ready():
	if DisplayServer.get_name() != "headless":
		OS.open_midi_inputs()

	editor = $vst3_editor
	print("godot-vst3-host version: ", Vst3Server.get_version(), " build: ", Vst3Server.get_build())
	Vst3Server.layout_changed.connect(_on_layout_changed)
	Vst3Server.vst3_ready.connect(_on_vst3_ready)


func _on_layout_changed():
	pass


func _input(input_event):
	if input_event is InputEventMIDI:
		_send_midi_info(input_event)


func _send_midi_info(midi_event):
	if midi_event.message == MIDI_MESSAGE_NOTE_ON:
		vst3.note_on(bus, channel, midi_event.pitch, midi_event.velocity)
	if midi_event.message == MIDI_MESSAGE_NOTE_OFF:
		vst3.note_off(bus, channel, midi_event.pitch)
	if midi_event.message == MIDI_MESSAGE_CONTROL_CHANGE:
		vst3.control_change(bus, channel, midi_event.controller_number, midi_event.controller_value)


func _on_vst3_ready(_vst3_name: String):
	vst3 = Vst3Server.get_instance("Main")
	var input_parameters: Array[Vst3Parameter] = vst3.get_input_parameters()
	for input_parameter in input_parameters:
		pass

	for preset in vst3.get_presets():
		dropdown.add_item(preset)

	editor.initialize(vst3)


func _process(_delta):
	pass


func _on_check_button_toggled(toggled_on: bool):
	if toggled_on:
		vst3.note_on(bus, channel, 60, 90)
	else:
		vst3.note_off(bus, channel, 60)


func _on_check_button_2_toggled(toggled_on: bool):
	if toggled_on:
		vst3.note_on(bus, channel, 64, 90)
	else:
		vst3.note_off(bus, channel, 64)


func _on_check_button_3_toggled(toggled_on: bool):
	if toggled_on:
		vst3.note_on(bus, channel, 67, 90)
	else:
		vst3.note_off(bus, channel, 67)


func _on_v_slider_value_changed(value: float):
	pass


func _on_option_button_item_selected(index: int) -> void:
	var preset = dropdown.get_item_text(index)
	vst3.load_preset(preset)
	editor.update(vst3)
