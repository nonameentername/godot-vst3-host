#include "vst3_instance.h"
#include "godot_cpp/classes/audio_server.hpp"
#include "godot_cpp/classes/audio_stream_mp3.hpp"
#include "godot_cpp/classes/audio_stream_wav.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "vst3_parameter.h"
#include "vst3_server.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace godot;

namespace godot {

using Vst3Instance = godot::Vst3Instance;

Vst3Instance::Vst3Instance() {
    vst3_host = NULL;
    initialized = false;
    active = false;
    channels_cleared = false;

    finished = false;

    mix_rate = AudioServer::get_singleton()->get_mix_rate();

    // TODO: update block size
    int p_frames = 512;
    vst3_host = new Vst3Host(mix_rate, p_frames);

    mutex.instantiate();
    semaphore.instantiate();

    temp_buffer.resize(BUFFER_FRAME_SIZE);

    for (int i = 0; i < BUFFER_FRAME_SIZE; i++) {
        temp_buffer.ptrw()[i] = 0;
    }
}

void Vst3Instance::configure() {
    lock();

    if (!vst3_host->find_plugin(std::string(uri.ascii()))) {
        std::cerr << "Plugin not found: " << uri.ascii() << "\n";
    }

    if (!vst3_host->instantiate()) {
        std::cerr << "Failed to instantiate plugin\n";
    }

    std::vector<std::pair<std::string, float>> cli_sets;

    vst3_host->wire_worker_interface();
    vst3_host->set_cli_parameter_overrides(cli_sets);

    // TODO: use the block from godot instead of 512
    int p_frames = 512;
    if (!vst3_host->prepare_ports_and_buffers(p_frames)) {
        std::cerr << "Failed to prepare/connect ports\n";
    }

    input_parameters.clear();

    for (int i = 0; i < vst3_host->get_input_parameter_count(); i++) {
        const host::Vst3Parameter *host_parameter = vst3_host->get_input_parameter(i);
        Vst3Parameter *parameter = memnew(Vst3Parameter);
        parameter->set_index(i);
        parameter->set_id(host_parameter->id);
        parameter->set_title(host_parameter->title.c_str());
        parameter->set_short_title(host_parameter->short_title.c_str());
        parameter->set_units(host_parameter->units.c_str());
        parameter->set_step_count(host_parameter->step_count);
        parameter->set_default_normalized_value(host_parameter->default_normalized_value);
        parameter->set_unit_id(host_parameter->unit_id);
        parameter->set_can_automate(host_parameter->can_automate);
        parameter->set_is_read_only(host_parameter->is_read_only);
        parameter->set_is_wrap_around(host_parameter->is_wrap_around);
        parameter->set_is_list(host_parameter->is_list);
        parameter->set_is_hidden(host_parameter->is_hidden);
        parameter->set_is_program_change(host_parameter->is_program_change);
        parameter->set_is_bypass(host_parameter->is_bypass);

        input_parameters.append(parameter);
    }

    output_parameters.clear();

    for (int i = 0; i < vst3_host->get_output_parameter_count(); i++) {
        const host::Vst3Parameter *host_parameter = vst3_host->get_output_parameter(i);
        Vst3Parameter *parameter = memnew(Vst3Parameter);
        //TODO: remove duplicate code
        parameter->set_index(i);
        parameter->set_id(host_parameter->id);
        parameter->set_title(host_parameter->title.c_str());
        parameter->set_short_title(host_parameter->short_title.c_str());
        parameter->set_units(host_parameter->units.c_str());
        parameter->set_step_count(host_parameter->step_count);
        parameter->set_default_normalized_value(host_parameter->default_normalized_value);
        parameter->set_unit_id(host_parameter->unit_id);
        parameter->set_can_automate(host_parameter->can_automate);
        parameter->set_is_read_only(host_parameter->is_read_only);
        parameter->set_is_wrap_around(host_parameter->is_wrap_around);
        parameter->set_is_list(host_parameter->is_list);
        parameter->set_is_hidden(host_parameter->is_hidden);
        parameter->set_is_program_change(host_parameter->is_program_change);
        parameter->set_is_bypass(host_parameter->is_bypass);

        output_parameters.append(parameter);
    }

    input_channels.resize(vst3_host->get_input_channel_count());
    output_channels.resize(vst3_host->get_output_channel_count());

    std::vector<std::string> host_presets = vst3_host->get_presets();

    presets.clear();

    for (int i = 0; i < host_presets.size(); i++) {
        presets.push_back(host_presets[i].c_str());
    }

    unlock();
}

Vst3Instance::~Vst3Instance() {
    if (vst3_host != NULL) {
        delete vst3_host;
        vst3_host = NULL;
    }
}

void Vst3Instance::start() {
    if (vst3_host != NULL) {
        vst3_host->activate();

        initialized = true;
        start_thread();

        emit_signal("vst3_ready", instance_name);
    }
}

void Vst3Instance::stop() {
    bool prev_initialized = initialized;
    initialized = false;
    stop_thread();

    if (vst3_host != NULL) {
        vst3_host->deactivate();
        if (prev_initialized) {
            cleanup_channels();
        }
    }
}

void Vst3Instance::finish() {
    stop();
    finished = true;
}

void Vst3Instance::reset() {
    bool prev_initialized = initialized;
    initialized = false;
    stop_thread();

    if (vst3_host != NULL) {
        if (prev_initialized) {
            cleanup_channels();
        }
    }
}

void Vst3Instance::cleanup_channels() {
    lock();

    input_channels.clear();
    output_channels.clear();

    unlock();
}

int Vst3Instance::process_sample(AudioFrame *p_buffer, float p_rate, int p_frames) {
    if (finished) {
        return 0;
    }

    lock();

    int read = p_frames;

    if (Time::get_singleton()) {
        last_mix_time = Time::get_singleton()->get_ticks_usec();
    }

    if (!initialized || output_channels.size() == 0) {
        for (int frame = 0; frame < p_frames; frame++) {
            p_buffer[frame].left = 0;
            p_buffer[frame].right = 0;
        }
    } else if (output_channels.size() > 1) {
        output_channels[0].buffer.read_channel(temp_buffer.ptrw(), p_frames);
        for (int frame = 0; frame < read; frame++) {
            p_buffer[frame].left = temp_buffer[frame];
        }

        output_channels[1].buffer.read_channel(temp_buffer.ptrw(), p_frames);
        for (int frame = 0; frame < read; frame++) {
            p_buffer[frame].right = temp_buffer[frame];
        }
    } else if (output_channels.size() > 0) {
        output_channels[0].buffer.read_channel(temp_buffer.ptrw(), p_frames);
        for (int frame = 0; frame < read; frame++) {
            p_buffer[frame].left = temp_buffer[frame];
            p_buffer[frame].right = temp_buffer[frame];
        }
    }

    for (int channel = 0; channel < output_channels.size(); channel++) {
        output_channels[channel].buffer.update_read_index(p_frames);
    }

    unlock();

    semaphore->post();

    return p_frames;
}

void Vst3Instance::set_channel_sample(AudioFrame *p_buffer, float p_rate, int p_frames, int left, int right) {
    bool has_left_channel = left >= 0 && left < input_channels.size();
    bool has_right_channel = right >= 0 && right < input_channels.size();

    if (!has_left_channel && !has_right_channel && !active) {
        return;
    }

    lock();

    if (has_left_channel) {
        for (int frame = 0; frame < p_frames; frame++) {
            temp_buffer.ptrw()[frame] = p_buffer[frame].left;
        }

        input_channels[left].write_channel(temp_buffer.ptrw(), p_frames);
    }

    if (has_right_channel) {
        for (int frame = 0; frame < p_frames; frame++) {
            temp_buffer.ptrw()[frame] = p_buffer[frame].right;
        }

        input_channels[right].write_channel(temp_buffer.ptrw(), p_frames);
    }

    unlock();
}

int Vst3Instance::get_channel_sample(AudioFrame *p_buffer, float p_rate, int p_frames, int left, int right) {
    bool has_left_channel = left >= 0 && left < output_channels.size();
    bool has_right_channel = right >= 0 && right < output_channels.size();

    if (finished) {
        return 0;
    }

    lock();

    if (has_left_channel && active) {
        output_channels[left].buffer.read_channel(temp_buffer.ptrw(), p_frames);
        for (int frame = 0; frame < p_frames; frame++) {
            p_buffer[frame].left = temp_buffer[frame];
        }
    } else {
        for (int frame = 0; frame < p_frames; frame++) {
            p_buffer[frame].left = 0;
        }
    }
    if (has_right_channel && active) {
        output_channels[right].buffer.read_channel(temp_buffer.ptrw(), p_frames);
        for (int frame = 0; frame < p_frames; frame++) {
            p_buffer[frame].right = temp_buffer[frame];
        }
    } else {
        for (int frame = 0; frame < p_frames; frame++) {
            p_buffer[frame].right = 0;
        }
    }

    unlock();

    return p_frames;
}

void Vst3Instance::program_select(int chan, int bank_num, int preset_num) {
}

void Vst3Instance::note_on(int midi_bus, int chan, int key, int vel) {
    if (!initialized) {
        return;
    }

    MidiEvent event;

    if (vel > 0) {
        event.data[0] = (MIDIMessage::MIDI_MESSAGE_NOTE_ON << 4) | (chan & 0x0F);
    } else {
        event.data[0] = (MIDIMessage::MIDI_MESSAGE_NOTE_OFF << 4) | (chan & 0x0F);
    }

    event.data[1] = key;
    event.data[2] = vel;

    vst3_host->write_midi_in(midi_bus, event);
}

void Vst3Instance::note_off(int midi_bus, int chan, int key) {
    if (!initialized) {
        return;
    }

    MidiEvent event;
    event.data[0] = (MIDIMessage::MIDI_MESSAGE_NOTE_OFF << 4) | (chan & 0x0F);
    event.data[1] = key;
    event.data[2] = 0;

    vst3_host->write_midi_in(midi_bus, event);
}

void Vst3Instance::control_change(int midi_bus, int chan, int control, int value) {
    if (!initialized) {
        return;
    }

    MidiEvent event;
    event.data[0] = (MIDIMessage::MIDI_MESSAGE_CONTROL_CHANGE << 4) | (chan & 0x0F);
    event.data[1] = control;
    event.data[2] = value;

    vst3_host->write_midi_in(midi_bus, event);
}

void Vst3Instance::send_input_parameter_channel(int p_channel, float p_value) {
    if (!initialized) {
        return;
    }

    vst3_host->set_input_parameter_value(p_channel, p_value);
}

float Vst3Instance::get_input_parameter_channel(int p_channel) {
    if (!initialized) {
        return 0;
    }

    return vst3_host->get_input_parameter_value(p_channel);
}

void Vst3Instance::send_output_parameter_channel(int p_channel, float p_value) {
    if (!initialized) {
        return;
    }

    vst3_host->set_output_parameter_value(p_channel, p_value);
}

float Vst3Instance::get_output_parameter_channel(int p_channel) {
    if (!initialized) {
        return 0;
    }

    return vst3_host->get_output_parameter_value(p_channel);
}

void Vst3Instance::pitch_bend(int chan, int val) {
}

void Vst3Instance::thread_func() {
    int p_frames = 512;

    while (!exit_thread) {
        if (!initialized) {
            continue;
        }

        last_mix_frames = p_frames;

        lock();

        float volume = godot::UtilityFunctions::db_to_linear(volume_db);

        if (Vst3Server::get_singleton()->get_solo_mode()) {
            if (!solo) {
                volume = 0.0;
            }
        } else {
            if (mute) {
                volume = 0.0;
            }
        }

        Vector<float> channel_peak;
        channel_peak.resize(output_channels.size());
        for (int i = 0; i < output_channels.size(); i++) {
            channel_peak.ptrw()[i] = 0;
        }

        for (int channel = 0; channel < vst3_host->get_input_channel_count(); channel++) {
            input_channels[channel].read_channel(temp_buffer.ptrw(), p_frames);

            for (int frame = 0; frame < p_frames; frame++) {
                vst3_host->get_input_channel_buffer(channel)[frame] = temp_buffer[frame];
            }

            input_channels[channel].update_read_index(p_frames);
        }

        int result = vst3_host->perform(p_frames);
        if (result == 0) {
            finished = true;
        }

        if (bypass) {
            for (int channel = 0; channel < vst3_host->get_output_channel_count(); channel++) {
                if (channel < input_channels.size()) {
                    input_channels[channel].read_channel(temp_buffer.ptrw(), p_frames);
                } else {
                    for (int frame = 0; frame < p_frames; frame++) {
                        temp_buffer.ptrw()[frame] = 0;
                    }
                }
                output_channels[channel].buffer.write_channel(temp_buffer.ptr(), p_frames);
            }
        } else {
            for (int channel = 0; channel < vst3_host->get_output_channel_count(); channel++) {
                for (int frame = 0; frame < p_frames; frame++) {
                    float value = vst3_host->get_output_channel_buffer(channel)[frame] * volume;
                    float p = Math::abs(value);
                    if (p > channel_peak[channel]) {
                        channel_peak.ptrw()[channel] = p;
                    }
                    temp_buffer.ptrw()[frame] = value;
                }
                output_channels[channel].buffer.write_channel(temp_buffer.ptr(), p_frames);
            }
        }

        for (int channel = 0; channel < output_channels.size(); channel++) {
            output_channels[channel].peak_volume =
                godot::UtilityFunctions::linear_to_db(channel_peak[channel] + AUDIO_PEAK_OFFSET);

            if (channel_peak[channel] > 0) {
                output_channels[channel].active = true;
            } else {
                output_channels[channel].active = false;
            }
        }

        unlock();

        semaphore->wait();
    }
}

Error Vst3Instance::start_thread() {
    if (thread.is_null()) {
        thread.instantiate();
        exit_thread = false;
        thread->start(callable_mp(this, &Vst3Instance::thread_func), Thread::PRIORITY_HIGH);
    }
    return (Error)OK;
}

void Vst3Instance::stop_thread() {
    if (thread.is_valid()) {
        exit_thread = true;
        semaphore->post();
        thread->wait_to_finish();
        thread.unref();
    }
}

void Vst3Instance::lock() {
    if (thread.is_null() || mutex.is_null()) {
        return;
    }
    mutex->lock();
}

void Vst3Instance::unlock() {
    if (thread.is_null() || mutex.is_null()) {
        return;
    }
    mutex->unlock();
}

void Vst3Instance::initialize() {
    if (uri.length() > 0) {
        configure();
        start();
    }
}

void Vst3Instance::set_instance_name(const String &name) {
    instance_name = name;
}

const String &Vst3Instance::get_instance_name() {
    return instance_name;
}

void Vst3Instance::set_uri(const String &p_uri) {
    reset();
    uri = p_uri;
    configure();
    start();
}

int Vst3Instance::get_input_channel_count() {
    if (vst3_host != NULL) {
        return vst3_host->get_input_channel_count();
    } else {
        return 0;
    }
}

int Vst3Instance::get_output_channel_count() {
    if (vst3_host != NULL) {
        return vst3_host->get_output_channel_count();
    } else {
        return 0;
    }
}

int Vst3Instance::get_input_midi_count() {
    if (vst3_host != NULL) {
        return vst3_host->get_input_midi_count();
    } else {
        return 0;
    }
}

int Vst3Instance::get_output_midi_count() {
    if (vst3_host != NULL) {
        return vst3_host->get_output_midi_count();
    } else {
        return 0;
    }
}

TypedArray<Vst3Parameter> Vst3Instance::get_input_parameters() {
    return input_parameters;
}

TypedArray<Vst3Parameter> Vst3Instance::get_output_parameters() {
    return output_parameters;
}

TypedArray<String> Vst3Instance::get_presets() {
    return presets;
}

void Vst3Instance::load_preset(String p_preset) {
    vst3_host->load_preset(std::string(p_preset.ascii()));
}

double Vst3Instance::get_time_since_last_mix() {
    return (Time::get_singleton()->get_ticks_usec() - last_mix_time) / 1000000.0;
}

double Vst3Instance::get_time_to_next_mix() {
    double total = get_time_since_last_mix();
    double mix_buffer = last_mix_frames / AudioServer::get_singleton()->get_mix_rate();
    return mix_buffer - total;
}

void Vst3Instance::set_active(bool p_active) {
    active = p_active;
    channels_cleared = false;
    if (finished && p_active) {
        reset();
        finished = false;
        configure();
        start();
    }
}

bool Vst3Instance::is_active() {
    return active;
}

void Vst3Instance::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize"), &Vst3Instance::initialize);
    ClassDB::bind_method(D_METHOD("program_select", "chan", "bank_num", "preset_num"), &Vst3Instance::program_select);
    ClassDB::bind_method(D_METHOD("finish"), &Vst3Instance::finish);

    ClassDB::bind_method(D_METHOD("note_on", "chan", "key", "vel"), &Vst3Instance::note_on);
    ClassDB::bind_method(D_METHOD("note_off", "chan", "key"), &Vst3Instance::note_off);
    ClassDB::bind_method(D_METHOD("control_change", "chan", "control", "key"), &Vst3Instance::control_change);

    ClassDB::bind_method(D_METHOD("send_input_parameter_channel", "channel", "value"),
                         &Vst3Instance::send_input_parameter_channel);
    ClassDB::bind_method(D_METHOD("get_input_parameter_channel", "channel"), &Vst3Instance::get_input_parameter_channel);

    ClassDB::bind_method(D_METHOD("send_output_parameter_channel", "channel", "value"),
                         &Vst3Instance::send_output_parameter_channel);
    ClassDB::bind_method(D_METHOD("get_output_parameter_channel", "channel"), &Vst3Instance::get_output_parameter_channel);

    ClassDB::bind_method(D_METHOD("pitch_bend", "chan", "vel"), &Vst3Instance::pitch_bend);

    ClassDB::bind_method(D_METHOD("set_instance_name", "name"), &Vst3Instance::set_instance_name);
    ClassDB::bind_method(D_METHOD("get_instance_name"), &Vst3Instance::get_instance_name);

    ClassDB::bind_method(D_METHOD("get_input_parameters"), &Vst3Instance::get_input_parameters);
    ClassDB::bind_method(D_METHOD("get_output_parameters"), &Vst3Instance::get_output_parameters);

    ClassDB::bind_method(D_METHOD("get_presets"), &Vst3Instance::get_presets);
    ClassDB::bind_method(D_METHOD("load_preset", "preset"), &Vst3Instance::load_preset);

    ClassDB::add_property("Vst3Instance", PropertyInfo(Variant::STRING, "instance_name"), "set_instance_name",
                          "get_instance_name");

    ADD_SIGNAL(MethodInfo("vst3_ready", PropertyInfo(Variant::STRING, "name")));
}

} // namespace godot
