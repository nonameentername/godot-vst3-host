@tool
extends PanelContainer
class_name EditorVst3Instance

var editor_interface: EditorInterface
var dark_theme = false
var audio_meter: EditorAudioMeterNotchesVst3
var audio_meter2: EditorAudioMeterNotchesVst3
var audio_value_preview_box: Panel
var audio_value_preview_label: Label
var solo: Button
var mute: Button
var bypass: Button
var options: MenuButton
var tree: Tree
var add_intrument: TreeItem
var slider: VSlider
var preview_timer: Timer
var initialized = false
var updating_instance: bool
var vst3_name: LineEdit
var is_main: bool
var enabled_vu: Texture2D
var disabled_vu: Texture2D
var active_bus_texture: Texture2D
var hovering_drop: bool
var editor_instances: EditorVst3Instances
var undo_redo: EditorUndoRedoManager
var channel_container: BoxContainer
var vst3_file_label: LineEdit
var vst3_file_hbox: HBoxContainer
var plugin_option: OptionButton
var plugin_info: Array


class Channel:
	var prev_active: bool
	var peak: float = 0
	var vu: TextureProgressBar


var channels: Array[Channel]
var named_channels: Array[Channel]


func _init():
	add_user_signal("duplicate_request")
	add_user_signal("delete_request")
	add_user_signal("vol_reset_request")
	add_user_signal("drop_end_request")
	add_user_signal("dropped")

	updating_instance = false
	hovering_drop = false


func _ready():
	vst3_name = $VBoxContainer/LineEdit
	channel_container = $VBoxContainer/HBoxContainer2/Channels
	slider = $VBoxContainer/HBoxContainer2/Channels/Slider
	audio_value_preview_box = $VBoxContainer/HBoxContainer2/Channels/Slider/AudioValuePreview
	audio_value_preview_label = $VBoxContainer/HBoxContainer2/Channels/Slider/AudioValuePreview/AudioPreviewHBox/AudioPreviewLabel
	solo = $VBoxContainer/HBoxContainer/LeftHBox/solo
	mute = $VBoxContainer/HBoxContainer/LeftHBox/mute
	bypass = $VBoxContainer/HBoxContainer/LeftHBox/bypass
	options = $VBoxContainer/HBoxContainer/RightHBox/options
	tree = $VBoxContainer/Tree
	vst3_file_label = $VBoxContainer/HBoxContainer3/LeftHBox/Label
	vst3_file_hbox = $VBoxContainer/HBoxContainer3/RightHBox
	preview_timer = $Timer
	plugin_option = $VBoxContainer/HBoxContainer3/LeftHBox/PluginOption
	plugin_info = []

	var custom_theme = Theme.new()
	var empty_texture = ImageTexture.new()
	empty_texture.create_placeholder()

	custom_theme.set_icon("radio_checked", "PopupMenu", empty_texture)
	custom_theme.set_icon("radio_unchecked", "PopupMenu", empty_texture)

	var popup_menu = plugin_option.get_popup()
	popup_menu.theme = custom_theme

	plugin_option.clear()

	var plugin_index: int = 0
	for plugin_uri in Vst3Server.get_plugins():
		plugin_info.push_back(plugin_uri)
		plugin_option.add_item(Vst3Server.get_plugin_name(plugin_uri), plugin_index)

	initialized = true

	focus_mode = Control.FOCUS_CLICK

	options.shortcut_context = self
	var popup = options.get_popup()

	popup.clear()

	var input_event: InputEventKey = InputEventKey.new()
	input_event.ctrl_pressed = true
	input_event.command_or_control_autoremap = true
	input_event.keycode = KEY_D
	var shortcut: Shortcut = Shortcut.new()
	shortcut.events = [input_event]
	shortcut.resource_name = "Duplicate Vst3 Plugin"

	popup.add_shortcut(shortcut, 1)

	input_event = InputEventKey.new()
	input_event.keycode = KEY_DELETE
	shortcut = Shortcut.new()
	shortcut.events = [input_event]
	shortcut.resource_name = "Delete Vst3 Plugin"
	popup.add_shortcut(shortcut)
	popup.set_item_disabled(1, is_main)

	shortcut = Shortcut.new()
	shortcut.resource_name = "Reset volume"
	popup.add_shortcut(shortcut)

	popup.connect("index_pressed", _popup_pressed)

	tree.hide_root = true
	tree.hide_folding = true
	tree.allow_rmb_select = true
	tree.focus_mode = FOCUS_CLICK
	tree.allow_reselect = true

	update_instance()
	_update_theme()


func _create_channel_progress_bar():
	var channel_progress_bar: TextureProgressBar = TextureProgressBar.new()
	channel_progress_bar.fill_mode = TextureProgressBar.FILL_BOTTOM_TO_TOP
	channel_progress_bar.min_value = -80
	channel_progress_bar.max_value = 24
	channel_progress_bar.step = 0.1

	channel_progress_bar.texture_under = enabled_vu
	channel_progress_bar.tint_under = Color(0.75, 0.75, 0.75)
	channel_progress_bar.texture_progress = enabled_vu

	return channel_progress_bar


func _create_channel():
	var channel: Channel = Channel.new()
	channel.peak = -200
	channel.prev_active = false

	return channel


func _update_visiable_channels():
	for channel in channel_container.get_children():
		if is_instance_of(channel, TextureProgressBar):
			channel_container.remove_child(channel)

	channels.clear()
	channels.resize(Vst3Server.get_channel_count(get_index()))

	for i in range(0, Vst3Server.get_channel_count(get_index())):
		var channel: Channel = _create_channel()
		var channel_progress_bar: TextureProgressBar = _create_channel_progress_bar()
		channel.vu = channel_progress_bar
		channels[i] = channel
		channel_container.add_child(channel_progress_bar)

	channel_container.move_child(audio_meter, channel_container.get_child_count())

	named_channels.clear()


func _process(_delta):
	if channels.size() != Vst3Server.get_channel_count(get_index()):
		_update_visiable_channels()

	for i in range(0, channels.size()):
		var real_peak = -100

		if Vst3Server.is_channel_active(get_index(), i):
			real_peak = max(real_peak, Vst3Server.get_channel_peak_volume_db(get_index(), i))

		_update_channel_vu(channels[i], real_peak)


func _update_channel_vu(channel, real_peak):
	if real_peak > channel.peak:
		channel.peak = real_peak
	else:
		channel.peak -= get_process_delta_time() * 60.0
	channel.vu.value = channel.peak

	if _scaled_db_to_normalized_volume(channel.peak) > 0:
		channel.vu.texture_over = null
	else:
		channel.vu.texture_over = disabled_vu


func _notification(what):
	match what:
		NOTIFICATION_THEME_CHANGED:
			if not initialized:
				return
			_update_theme()

		NOTIFICATION_DRAW:
			if is_main:
				draw_style_box(get_theme_stylebox("disabled", "Button"), Rect2(Vector2(), size))
			elif has_focus():
				draw_style_box(get_theme_stylebox("focus", "Button"), Rect2(Vector2(), size))
			else:
				draw_style_box(get_theme_stylebox("panel", "TabContainer"), Rect2(Vector2(), size))

			if get_index() != 0 && hovering_drop:
				var accent: Color = get_theme_color("accent_color", "Editor")
				accent.a *= 0.7
				draw_rect(Rect2(Vector2(), size), accent, false)

		NOTIFICATION_PROCESS:
			pass

		NOTIFICATION_MOUSE_EXIT:
			if hovering_drop:
				hovering_drop = false
				queue_redraw()

		NOTIFICATION_DRAG_END:
			if hovering_drop:
				hovering_drop = false
				queue_redraw()


func _hide_value_preview():
	audio_value_preview_box.hide()


func _show_value(value):
	var db: float
	if Input.is_key_pressed(KEY_CTRL):
		db = round(_normalized_volume_to_scaled_db(value))
	else:
		db = _normalized_volume_to_scaled_db(value)

	var text

	if is_zero_approx(snapped(db, 0.1)):
		text = " 0.0 dB"
	else:
		text = "%.1f dB" % db

	slider.tooltip_text = text
	audio_value_preview_label.text = text
	var slider_size: Vector2 = slider.size
	var slider_position: Vector2 = slider.global_position
	var vert_padding: float = 10.0
	var box_position: Vector2 = Vector2(
		slider_size.x, (slider_size.y - vert_padding) * (1.0 - slider.value) - vert_padding
	)
	audio_value_preview_box.position = slider_position + box_position
	audio_value_preview_box.size = audio_value_preview_label.size

	if slider.has_focus() and !audio_value_preview_box.visible:
		audio_value_preview_box.show()
	preview_timer.start()


func _tab_changed(tab: int):
	if updating_instance:
		return

	_update_slider()


func _value_changed(normalized):
	if updating_instance:
		return

	updating_instance = true

	var db: float = _normalized_volume_to_scaled_db(normalized)
	if Input.is_key_pressed(KEY_CTRL):
		slider.value = _scaled_db_to_normalized_volume(round(db))

	undo_redo.create_action("Change Vst3 Volume", UndoRedo.MERGE_ENDS)
	undo_redo.add_do_method(Vst3Server, "set_volume_db", get_index(), db)
	undo_redo.add_undo_method(
		Vst3Server, "set_volume_db", get_index(), Vst3Server.get_volume_db(get_index())
	)
	undo_redo.add_do_method(editor_instances, "_update_instance", get_index())
	undo_redo.add_undo_method(editor_instances, "_update_instance", get_index())
	undo_redo.commit_action()

	updating_instance = false


func _normalized_volume_to_scaled_db(normalized):
	if normalized > 0.6:
		return 22.22 * normalized - 16.2
	elif normalized < 0.05:
		return 830.72 * normalized - 80.0
	else:
		return 45.0 * pow(normalized - 1.0, 3)


func _scaled_db_to_normalized_volume(db):
	if db > -2.88:
		return (db + 16.2) / 22.22
	elif db < -38.602:
		return (db + 80.0) / 830.72
	else:
		if db < 0.0:
			var positive_x: float = pow(abs(db) / 45.0, 1.0 / 3.0) + 1.0
			var translation: Vector2 = Vector2(1.0, 0.0) - Vector2(positive_x, abs(db))
			var reflected_position: Vector2 = Vector2(1.0, 0.0) + translation
			return reflected_position.x
		else:
			return pow(db / 45.0, 1.0 / 3.0) + 1.0


func _update_theme():
	if not editor_interface:
		return

	var base_color: Color = EditorInterface.get_editor_settings().get_setting(
		"interface/theme/base_color"
	)

	dark_theme = base_color.v < .5

	var solo_color = Color(1.0, 0.89, 0.22) if dark_theme else Color(1.0, 0.92, 0.44)
	solo.add_theme_color_override("icon_pressed_color", solo_color)
	solo.icon = get_theme_icon("AudioBusSolo", "EditorIcons")

	var mute_color = Color(1.0, 0.16, 0.16) if dark_theme else Color(1.0, 0.44, 0.44)
	mute.add_theme_color_override("icon_pressed_color", mute_color)
	mute.icon = get_theme_icon("AudioBusMute", "EditorIcons")

	var bypass_color = Color(0.13, 0.8, 1.0) if dark_theme else Color(0.44, 0.87, 1.0)
	bypass.add_theme_color_override("icon_pressed_color", bypass_color)
	bypass.icon = get_theme_icon("AudioBusBypass", "EditorIcons")

	options.icon = get_theme_icon("GuiTabMenuHl", "EditorIcons")

	audio_value_preview_label.add_theme_color_override(
		"font_color", get_theme_color("font_color", "TooltipLabel")
	)
	audio_value_preview_label.add_theme_color_override(
		"font_shadow_color", get_theme_color("font_shadow_color", "TooltipLabel")
	)
	audio_value_preview_box.add_theme_stylebox_override(
		"panel", get_theme_stylebox("panel", "TooltipPanel")
	)

	tree.custom_minimum_size = Vector2(0, 80) * EditorInterface.get_editor_scale()
	tree.clear()

	var root: TreeItem = tree.create_item()

	tree.button_clicked.connect(_tree_button_clicked)
	tree.custom_item_clicked.connect(_tree_custom_item_clicked)

	enabled_vu = get_theme_icon("BusVuActive", "EditorIcons")
	disabled_vu = get_theme_icon("BusVuFrozen", "EditorIcons")

	if not is_instance_valid(audio_meter):
		audio_meter = EditorAudioMeterNotchesVst3.new()
		channel_container.add_child(audio_meter)

	if not is_instance_valid(audio_meter2):
		audio_meter2 = EditorAudioMeterNotchesVst3.new()


func _tree_button_clicked(item: TreeItem, _column: int, _id: int, _mouse_button_index: int):
	if item == add_intrument:
		pass
	else:
		print("gone clicking")


func _tree_custom_item_clicked(mouse_button_index: int):
	if mouse_button_index == MOUSE_BUTTON_LEFT:
		pass
	elif mouse_button_index == MOUSE_BUTTON_RIGHT:
		pass
	print("adding an instrument", mouse_button_index, MOUSE_BUTTON_RIGHT)


func _get_drag_data(at_position):
	if get_index() == 0:
		return

	var control: Control = Control.new()
	var panel: Panel = Panel.new()
	control.add_child(panel)
	panel.modulate = Color(1, 1, 1, 0.7)
	panel.add_theme_stylebox_override("panel", get_theme_stylebox("focus", "Button"))
	panel.set_size(get_size())
	panel.set_position(-at_position)
	set_drag_preview(control)
	var dictionary: Dictionary = {"type": "move_instance", "index": get_index()}

	if get_index() < Vst3Server.get_instance_count() - 1:
		emit_signal("drop_end_request")

	return dictionary


func _can_drop_data(_at_position, data):
	if get_index() == 0:
		return false

	if data.has("type") and data["type"] == "move_instance" and int(data["index"]) != get_index():
		hovering_drop = true
		return true

	return false


func _drop_data(_at_position, data):
	emit_signal("dropped", data["index"], get_index())


func _popup_pressed(option: int):
	#if not has_focus():
	#	return
	match option:
		2:
			emit_signal("vol_reset_request")
		1:
			emit_signal("delete_request")
		0:
			emit_signal("duplicate_request", get_index())


func update_instance():
	if updating_instance:
		return

	updating_instance = true

	var index: int = get_index()

	var db_value: float = Vst3Server.get_volume_db(index)
	slider.value = _scaled_db_to_normalized_volume(db_value)
	vst3_name.text = Vst3Server.get_instance_name(index)

	if is_main:
		vst3_name.editable = false

	solo.button_pressed = Vst3Server.is_solo(index)
	mute.button_pressed = Vst3Server.is_mute(index)
	bypass.button_pressed = Vst3Server.is_bypassing(index)

	var vst3_uri = Vst3Server.get_uri(index)

	if vst3_uri:
		vst3_file_label.text = vst3_uri
		for info_index in plugin_info.size():
			if plugin_info[info_index] == vst3_uri:
				plugin_option.select(info_index)
	else:
		vst3_file_label.text = "<empty>"

	_update_slider()

	updating_instance = false


func _update_slider():
	pass


func _solo_toggled():
	updating_instance = true

	undo_redo.create_action("Toggle Vst3 Solo")
	undo_redo.add_do_method(Vst3Server, "set_solo", get_index(), solo.is_pressed())
	undo_redo.add_undo_method(Vst3Server, "set_solo", get_index(), Vst3Server.is_solo(get_index()))
	undo_redo.add_do_method(editor_instances, "_update_instance", get_index())
	undo_redo.add_undo_method(editor_instances, "_update_instance", get_index())
	undo_redo.commit_action()

	updating_instance = false


func _mute_toggled():
	updating_instance = true

	undo_redo.create_action("Toggle Vst3 mute")
	undo_redo.add_do_method(Vst3Server, "set_mute", get_index(), mute.is_pressed())
	undo_redo.add_undo_method(Vst3Server, "set_mute", get_index(), Vst3Server.is_mute(get_index()))
	undo_redo.add_do_method(editor_instances, "_update_instance", get_index())
	undo_redo.add_undo_method(editor_instances, "_update_instance", get_index())
	undo_redo.commit_action()

	updating_instance = false


func _bypass_toggled():
	updating_instance = true

	undo_redo.create_action("Toggle Vst3 bypass")
	undo_redo.add_do_method(Vst3Server, "set_bypass", get_index(), bypass.is_pressed())
	undo_redo.add_undo_method(
		Vst3Server, "set_bypass", get_index(), Vst3Server.is_bypassing(get_index())
	)
	undo_redo.add_do_method(editor_instances, "_update_instance", get_index())
	undo_redo.add_undo_method(editor_instances, "_update_instance", get_index())
	undo_redo.commit_action()

	updating_instance = false


func _name_changed(new_name: String):
	if updating_instance:
		return

	updating_instance = true
	#vst3_name.release_focus()

	if new_name == Vst3Server.get_instance_name(get_index()):
		updating_instance = false
		return new_name

	var attempt: String = new_name
	var attempts: int = 1

	while true:
		var name_free: bool = true
		for i in range(0, Vst3Server.get_instance_count()):
			if Vst3Server.get_instance_name(i) == attempt:
				name_free = false
				break

		if name_free:
			break

		attempts += 1
		attempt = new_name + " " + str(attempts)

	var current: String = Vst3Server.get_instance_name(get_index())

	undo_redo.create_action("Rename Vst3 Plugin")
	undo_redo.add_do_method(editor_instances, "_set_renaming", true)
	undo_redo.add_undo_method(editor_instances, "_set_renaming", true)

	undo_redo.add_do_method(Vst3Server, "set_instance_name", get_index(), attempt)
	undo_redo.add_undo_method(Vst3Server, "set_instance_name", get_index(), current)

	undo_redo.add_do_method(editor_instances, "_update_instance", get_index())
	undo_redo.add_undo_method(editor_instances, "_update_instance", get_index())

	undo_redo.add_do_method(editor_instances, "_set_renaming", false)
	undo_redo.add_undo_method(editor_instances, "_set_renaming", false)
	undo_redo.commit_action()

	updating_instance = false

	return attempt


func _resource_changed(resource: Resource):
	pass


func _name_focus_exit():
	vst3_name.text = _name_changed(vst3_name.get_text())


func resource_saved(resource: Resource):
	pass


func _resource_selected(resource: Resource, _inspect: bool):
	EditorInterface.edit_resource(resource)


func _on_plugin_option_item_selected(index: int) -> void:
	updating_instance = true

	undo_redo.create_action("Set Vst3 uri")
	undo_redo.add_do_method(Vst3Server, "set_uri", get_index(), plugin_info[index])
	undo_redo.add_undo_method(Vst3Server, "set_uri", get_index(), Vst3Server.get_uri(get_index()))
	undo_redo.add_do_method(editor_instances, "_update_instance", get_index())
	undo_redo.add_undo_method(editor_instances, "_update_instance", get_index())
	undo_redo.commit_action()

	updating_instance = false
