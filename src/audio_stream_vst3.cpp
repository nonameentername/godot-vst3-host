#include "audio_stream_vst3.h"
#include "audio_stream_player_vst3.h"
#include "godot_cpp/classes/audio_stream.hpp"
#include "vst3_server.h"

namespace godot {

AudioStreamVst3::AudioStreamVst3() {
    Vst3Server::get_singleton()->connect("layout_changed", Callable(this, "on_layout_changed"));
    Vst3Server::get_singleton()->connect("vst3_ready", Callable(this, "on_vst3_ready"));
    active = false;
}

AudioStreamVst3::~AudioStreamVst3() {
}

Ref<AudioStreamPlayback> AudioStreamVst3::_instantiate_playback() const {
    Ref<AudioStreamPlaybackVst3> talking_tree;
    talking_tree.instantiate();
    talking_tree->base = Ref<AudioStreamVst3>(this);

    return talking_tree;
}

void AudioStreamVst3::set_active(bool p_active) {
    active = p_active;

    Vst3Instance *vst3_instance = get_instance();
    if (vst3_instance != NULL) {
        vst3_instance->set_active(active);
    }
}

bool AudioStreamVst3::is_active() {
    Vst3Instance *vst3_instance = get_instance();
    if (vst3_instance != NULL) {
        return vst3_instance->is_active();
    }
    return false;
}

void AudioStreamVst3::set_instance_name(const String &name) {
    vst3_name = name;
}

const String &AudioStreamVst3::get_instance_name() const {
    static const String default_name = "Main";
    if (vst3_name.length() == 0) {
        return default_name;
    }

    for (int i = 0; i < Vst3Server::get_singleton()->get_instance_count(); i++) {
        if (Vst3Server::get_singleton()->get_instance_name(i) == vst3_name) {
            return vst3_name;
        }
    }

    return default_name;
}

String AudioStreamVst3::get_stream_name() const {
    return "Vst3";
}

float AudioStreamVst3::get_length() const {
    return 0;
}

bool AudioStreamVst3::_set(const StringName &p_name, const Variant &p_value) {
    if ((String)p_name == "vst3_name") {
        set_instance_name(p_value);
        return true;
    }
    return false;
}

bool AudioStreamVst3::_get(const StringName &p_name, Variant &r_ret) const {
    if ((String)p_name == "vst3_name") {
        r_ret = get_instance_name();
        return true;
    }
    return false;
}

void AudioStreamVst3::on_layout_changed() {
    notify_property_list_changed();
}

void AudioStreamVst3::on_vst3_ready(String p_vst3_name) {
    if (get_instance_name() == p_vst3_name) {
        Vst3Instance *vst3_instance = get_instance();
        if (vst3_instance != NULL) {
            vst3_instance->set_active(active);
        }
    }
}

Vst3Instance *AudioStreamVst3::get_instance() {
    Vst3Server *vst3_server = (Vst3Server *)Engine::get_singleton()->get_singleton("Vst3Server");
    if (vst3_server != NULL) {
        Vst3Instance *vst3_instance = vst3_server->get_instance(get_instance_name());
        return vst3_instance;
    }
    return NULL;
}

int AudioStreamVst3::process_sample(AudioFrame *p_buffer, float p_rate, int p_frames) {
    Vst3Instance *vst3_instance = get_instance();
    if (vst3_instance != NULL) {
        return vst3_instance->process_sample(p_buffer, p_rate, p_frames);
    }

    for (int frame = 0; frame < p_frames; frame += 1) {
        p_buffer[frame].left = 0;
        p_buffer[frame].right = 0;
    }

    return p_frames;
}

void AudioStreamVst3::_get_property_list(List<PropertyInfo> *p_list) const {
    String options = Vst3Server::get_singleton()->get_name_options();
    p_list->push_back(PropertyInfo(Variant::STRING_NAME, "vst3_name", PROPERTY_HINT_ENUM, options));
}

void AudioStreamVst3::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_stream_name"), &AudioStreamVst3::get_stream_name);
    ClassDB::bind_method(D_METHOD("set_instance_name", "name"), &AudioStreamVst3::set_instance_name);
    ClassDB::bind_method(D_METHOD("get_instance_name"), &AudioStreamVst3::get_instance_name);
    ClassDB::bind_method(D_METHOD("on_layout_changed"), &AudioStreamVst3::on_layout_changed);
    ClassDB::bind_method(D_METHOD("on_vst3_ready", "vst3_name"), &AudioStreamVst3::on_vst3_ready);
}

} // namespace godot
