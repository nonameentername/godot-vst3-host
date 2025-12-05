#ifndef VST3_HOST_H
#define VST3_HOST_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"

#include "vst3_circular_buffer.h"

namespace godot {

const int MIDI_BUFFER_SIZE = 2048;

struct CtrlSet {
    uint32_t index{};
    float value{};
};

struct AtomIn {
    uint32_t index{};
    bool midi{};
    std::vector<std::max_align_t> buf; // aligned storage
                                       // VST3_Atom_Sequence *seq{nullptr};
                                       // VST3_Atom_Forge forge{};
};

struct AtomOut {
    uint32_t index{};
    bool midi{};
    std::vector<std::max_align_t> buf; // aligned storage
                                       // VST3_Atom_Sequence *seq{nullptr};
};

struct MidiEvent {
    static constexpr const uint32_t DATA_SIZE = 3;

    int frame{};
    uint8_t data[DATA_SIZE];
    int size = DATA_SIZE;
};

namespace host {

struct Vst3Parameter {
    uint32_t id;
    std::string title;
    std::string short_title;
    std::string units;
    int step_count;
    int default_normalized_value;
    int unit_id;
    bool can_automate;
    bool is_read_only;
    bool is_wrap_around;
    bool is_list;
    bool is_hidden;
    bool is_program_change;
    bool is_bypass;
};

}; // namespace host

struct Vst3PluginInfo {
    std::string uri;
    std::string name;
};

struct ChannelMap {
    int bus;
    int channel;
};

struct Vst3ParameterBuffer {
    Steinberg::Vst::ParamID id{};
    double value = 0.0; // normalized [0..1]
    bool dirty = false;
};

struct State {
    // config
    double sample_rate{48000.0};
    int block_size{512};

    VST3::Hosting::ClassInfo chosen_class{};
    bool has_class = false;

    // hosting objects
    VST3::Hosting::Module::Ptr module;
    Steinberg::FUnknownPtr<Steinberg::Vst::IComponent> component;
    Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> processor;
    Steinberg::FUnknownPtr<Steinberg::Vst::IEditController> controller;

    Steinberg::Vst::PlugProvider *plug_provider = NULL;

    Steinberg::Vst::HostApplication host_app;

    // arrangements / activation flags
    Steinberg::Vst::SpeakerArrangement in_arr = Steinberg::Vst::SpeakerArr::kStereo;
    Steinberg::Vst::SpeakerArrangement out_arr = Steinberg::Vst::SpeakerArr::kStereo;
    bool active{false};
    bool processing{false};

    // per-call, reusable process structs
    Steinberg::Vst::ProcessData pd{};

    // temporary raw channel pointer arrays (point to user's buffers)
    float *in_chans[2] = {nullptr, nullptr};
    float *out_chans[2] = {nullptr, nullptr};

    State() {
        std::memset(&pd, 0, sizeof(pd));
    }
};

class Vst3Host {
private:
    // Ports / buffers
    std::vector<float *> port_buffers;
    std::vector<float *> cv_heap;

    std::vector<std::vector<float>> audio;
    std::vector<float *> audio_ptrs;
    std::vector<float *> audio_in_ptrs;
    std::vector<float *> audio_out_ptrs;
    // uint32_t channels{};

    std::vector<ChannelMap> audio_in_map;
    std::vector<ChannelMap> audio_out_map;

    std::vector<ChannelMap> midi_in_map;
    std::vector<ChannelMap> midi_out_map;

    // Atom ports
    std::vector<AtomIn> atom_inputs;
    std::vector<AtomOut> atom_outputs;
    uint32_t seq_capacity_hint{}; // BYTES

    // Midi buffers
    std::vector<Vst3CircularBuffer<int>> midi_input_buffer;
    std::vector<Vst3CircularBuffer<int>> midi_output_buffer;

    std::vector<Vst3ParameterBuffer> parameter_input_buffer;
    std::vector<Vst3ParameterBuffer> parameter_output_buffer;

    std::vector<host::Vst3Parameter> parameter_inputs;
    std::vector<host::Vst3Parameter> parameter_outputs;

    // CLI overrides
    std::vector<std::pair<std::string, float>> cli_sets;

    std::vector<std::string> default_search_paths;

    State state;

    // chosen plugin path + class
    std::string plugin_path;

	std::vector<std::string> get_vst3_recursive(const std::vector<std::string>& paths);
	void clear_plugin();

public:
    Vst3Host(double p_sr, int p_frames, uint32_t seq_bytes = 4096);
    ~Vst3Host();

    Vst3Host(const Vst3Host &) = delete;
    Vst3Host &operator=(const Vst3Host &) = delete;

    std::vector<Vst3PluginInfo> get_plugins_info();
    std::string get_plugin_name(std::string);

    bool find_plugin(const std::string &p_plugin_path);
    bool instantiate();
    void set_cli_parameter_overrides(const std::vector<std::pair<std::string, float>> &name_value_pairs);

    void dump_plugin_features() const;
    void dump_host_features() const;
    void dump_ports() const;

    bool prepare_ports_and_buffers(int p_frames);
    void activate();
    void deactivate();

    std::vector<std::string> get_presets();
    void load_preset(std::string preset);

    void wire_worker_interface();

    int perform(int p_frames);

    int get_input_channel_count();
    int get_output_channel_count();

    int get_input_midi_count();
    int get_output_midi_count();

    int get_input_parameter_count();
    int get_output_parameter_count();

    float *get_input_channel_buffer(int p_channel);
    float *get_output_channel_buffer(int p_channel);

    void write_midi_in(int p_bus, const MidiEvent &p_midi_event);
    bool read_midi_in(int p_bus, MidiEvent &p_midi_event);

    void write_midi_out(int p_bus, const MidiEvent &p_midi_event);
    bool read_midi_out(int p_bus, MidiEvent &p_midi_event);

    const host::Vst3Parameter *get_input_parameter(int p_index);
    const host::Vst3Parameter *get_output_parameter(int p_index);

    float get_input_parameter_value(int p_index);
    float get_output_parameter_value(int p_index);

    void set_input_parameter_value(int p_index, float p_value);
    void set_output_parameter_value(int p_index, float p_value);
};

} // namespace godot

#endif
