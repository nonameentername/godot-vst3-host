#include "vst3_layout.h"

using namespace godot;

bool Vst3Layout::_set(const StringName &p_name, const Variant &p_value) {
    String s = p_name;
    if (s.begins_with("vst3/")) {
        int index = s.get_slice("/", 1).to_int();
        if (instances.size() <= index) {
            instances.resize(index + 1);
        }

        Vst3 &vst3 = instances.write[index];

        String what = s.get_slice("/", 2);

        if (what == "name") {
            vst3.name = p_value;
        } else if (what == "solo") {
            vst3.solo = p_value;
        } else if (what == "mute") {
            vst3.mute = p_value;
        } else if (what == "bypass") {
            vst3.bypass = p_value;
        } else if (what == "volume_db") {
            vst3.volume_db = p_value;
        } else if (what == "uri") {
            vst3.uri = p_value;
        } else {
            return false;
        }

        return true;
    }

    return false;
}

bool Vst3Layout::_get(const StringName &p_name, Variant &r_ret) const {
    String s = p_name;
    if (s.begins_with("vst3/")) {
        int index = s.get_slice("/", 1).to_int();
        if (index < 0 || index >= instances.size()) {
            return false;
        }

        const Vst3 &vst3 = instances[index];

        String what = s.get_slice("/", 2);

        if (what == "name") {
            r_ret = vst3.name;
        } else if (what == "solo") {
            r_ret = vst3.solo;
        } else if (what == "mute") {
            r_ret = vst3.mute;
        } else if (what == "bypass") {
            r_ret = vst3.bypass;
        } else if (what == "volume_db") {
            r_ret = vst3.volume_db;
        } else if (what == "uri") {
            r_ret = vst3.uri;
        } else {
            return false;
        }

        return true;
    }

    return false;
}

void Vst3Layout::_get_property_list(List<PropertyInfo> *p_list) const {
    for (int i = 0; i < instances.size(); i++) {
        p_list->push_back(PropertyInfo(Variant::STRING, "vst3/" + itos(i) + "/name", PROPERTY_HINT_NONE, "",
                                       PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL));
        p_list->push_back(PropertyInfo(Variant::BOOL, "vst3/" + itos(i) + "/solo", PROPERTY_HINT_NONE, "",
                                       PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL));
        p_list->push_back(PropertyInfo(Variant::BOOL, "vst3/" + itos(i) + "/mute", PROPERTY_HINT_NONE, "",
                                       PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL));
        p_list->push_back(PropertyInfo(Variant::BOOL, "vst3/" + itos(i) + "/bypass", PROPERTY_HINT_NONE, "",
                                       PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL));
        p_list->push_back(PropertyInfo(Variant::FLOAT, "vst3/" + itos(i) + "/volume_db", PROPERTY_HINT_NONE, "",
                                       PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL));
        p_list->push_back(PropertyInfo(Variant::STRING, "vst3/" + itos(i) + "/uri", PROPERTY_HINT_NONE, "",
                                       PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL));
    }
}

Vst3Layout::Vst3Layout() {
    instances.resize(1);
    instances.write[0].name = "Main";
}

void Vst3Layout::_bind_methods() {
}
