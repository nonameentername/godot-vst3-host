#include "vst3_parameter.h"
#include "godot_cpp/classes/upnp.hpp"

using namespace godot;

Vst3Parameter::Vst3Parameter() {
}

Vst3Parameter::~Vst3Parameter() {
}

void Vst3Parameter::set_index(int p_index) {
    index = p_index;
}

int Vst3Parameter::get_index() {
    return index;
}

void Vst3Parameter::set_id(int p_id) {
    id = p_id;
}

int Vst3Parameter::get_id() {
    return id;
}

void Vst3Parameter::set_title(String p_title) {
    title = p_title;
}

String Vst3Parameter::get_title() {
    return title;
}

void Vst3Parameter::set_short_title(String p_short_title) {
    short_title = p_short_title;
}

String Vst3Parameter::get_short_title() {
    return short_title;
}

void Vst3Parameter::set_units(String p_units) {
    units = p_units;
}

String Vst3Parameter::get_units() {
    return units;
}

void Vst3Parameter::set_step_count(int p_step_count) {
    step_count = p_step_count;
}

int Vst3Parameter::get_step_count() {
    return step_count;
}

void Vst3Parameter::set_default_normalized_value(int p_default_normalized_value) {
    default_normalized_value = p_default_normalized_value;
}

int Vst3Parameter::get_default_normalized_value() {
    return default_normalized_value;
}

void Vst3Parameter::set_unit_id(int p_unit_id) {
    unit_id = p_unit_id;
}

int Vst3Parameter::get_unit_id() {
    return unit_id;
}

void Vst3Parameter::set_can_automate(bool p_can_automate) {
    can_automate = p_can_automate;
}

bool Vst3Parameter::get_can_automate() {
    return can_automate;
}

//TODO: maybe rename these get_is_ methods to get or is?
void Vst3Parameter::set_is_read_only(bool p_is_read_only) {
    is_read_only = p_is_read_only;
}

bool Vst3Parameter::get_is_read_only() {
    return is_read_only;
}

void Vst3Parameter::set_is_wrap_around(bool p_is_wrap_around) {
    is_wrap_around = p_is_wrap_around;
}

bool Vst3Parameter::get_is_wrap_around() {
    return is_wrap_around;
}

void Vst3Parameter::set_is_list(bool p_is_list) {
    is_list = p_is_list;
}

bool Vst3Parameter::get_is_list() {
    return is_list;
}

void Vst3Parameter::set_is_hidden(bool p_is_hidden) {
    is_hidden = p_is_hidden;
}

bool Vst3Parameter::get_is_hidden() {
    return is_hidden;
}

void Vst3Parameter::set_is_program_change(bool p_is_program_change) {
    is_program_change = p_is_program_change;
}

bool Vst3Parameter::get_is_program_change() {
    return is_program_change;
}

void Vst3Parameter::set_is_bypass(bool p_is_bypass) {
    is_bypass = p_is_bypass;
}

bool Vst3Parameter::get_is_bypass() {
    return is_bypass;
}

void Vst3Parameter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_index", "index"), &Vst3Parameter::set_index);
    ClassDB::bind_method(D_METHOD("get_index"), &Vst3Parameter::get_index);

    ClassDB::bind_method(D_METHOD("set_id", "id"), &Vst3Parameter::set_id);
    ClassDB::bind_method(D_METHOD("get_id"), &Vst3Parameter::get_id);

    ClassDB::bind_method(D_METHOD("set_title", "title"), &Vst3Parameter::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &Vst3Parameter::get_title);

    ClassDB::bind_method(D_METHOD("set_short_title", "short_title"), &Vst3Parameter::set_short_title);
    ClassDB::bind_method(D_METHOD("get_short_title"), &Vst3Parameter::get_short_title);

    ClassDB::bind_method(D_METHOD("set_units", "units"), &Vst3Parameter::set_units);
    ClassDB::bind_method(D_METHOD("get_units"), &Vst3Parameter::get_units);

    ClassDB::bind_method(D_METHOD("set_step_count", "step_count"), &Vst3Parameter::set_step_count);
    ClassDB::bind_method(D_METHOD("get_step_count"), &Vst3Parameter::get_step_count);

    ClassDB::bind_method(D_METHOD("set_default_normalized_value", "default_normalized_value"), &Vst3Parameter::set_default_normalized_value);
    ClassDB::bind_method(D_METHOD("get_default_normalized_value"), &Vst3Parameter::get_default_normalized_value);

    ClassDB::bind_method(D_METHOD("set_unit_id", "unit_id"), &Vst3Parameter::set_unit_id);
    ClassDB::bind_method(D_METHOD("get_unit_id"), &Vst3Parameter::get_unit_id);

    ClassDB::bind_method(D_METHOD("set_can_automate", "can_automate"), &Vst3Parameter::set_can_automate);
    ClassDB::bind_method(D_METHOD("get_can_automate"), &Vst3Parameter::get_can_automate);

    ClassDB::bind_method(D_METHOD("set_is_read_only", "is_read_only"), &Vst3Parameter::set_is_read_only);
    ClassDB::bind_method(D_METHOD("get_is_read_only"), &Vst3Parameter::get_is_read_only);

    ClassDB::bind_method(D_METHOD("set_is_wrap_around", "is_wrap_around"), &Vst3Parameter::set_is_wrap_around);
    ClassDB::bind_method(D_METHOD("get_is_wrap_around"), &Vst3Parameter::get_is_wrap_around);

    ClassDB::bind_method(D_METHOD("set_is_list", "is_list"), &Vst3Parameter::set_is_list);
    ClassDB::bind_method(D_METHOD("get_is_list"), &Vst3Parameter::get_is_list);

    ClassDB::bind_method(D_METHOD("set_is_hidden", "is_hidden"), &Vst3Parameter::set_is_hidden);
    ClassDB::bind_method(D_METHOD("get_is_hidden"), &Vst3Parameter::get_is_hidden);

    ClassDB::bind_method(D_METHOD("set_is_program_change", "is_program_change"), &Vst3Parameter::set_is_program_change);
    ClassDB::bind_method(D_METHOD("get_is_program_change"), &Vst3Parameter::get_is_program_change);

    ClassDB::bind_method(D_METHOD("set_is_bypass", "is_bypass"), &Vst3Parameter::set_is_bypass);
    ClassDB::bind_method(D_METHOD("get_is_bypass"), &Vst3Parameter::get_is_bypass);

    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "index"), "set_index", "get_index");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "short_title"), "set_short_title", "get_short_title");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "units"), "set_units", "get_units");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "step_count"), "set_step_count", "get_step_count");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "default_normalized_value"), "set_default_normalized_value", "get_default_normalized_value");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "unit_id"), "set_unit_id", "get_unit_id");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "can_automate"), "set_can_automate", "get_can_automate");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "is_read_only"), "set_is_read_only", "get_is_read_only");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "is_wrap_around"), "set_is_wrap_around", "get_is_wrap_around");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "is_list"), "set_is_list", "get_is_list");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "is_hidden"), "set_is_hidden", "get_is_hidden");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "is_program_change"), "set_is_program_change", "get_is_program_change");
    ClassDB::add_property("Vst3Parameter", PropertyInfo(Variant::STRING, "is_bypass"), "set_is_bypass", "get_is_bypass");
}
