#ifndef VST3_INSTANCE_H
#define VST3_INSTANCE_H

#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/semaphore.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/variant/typed_array.hpp"
#include "vst3_parameter.h"
#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vst3_circular_buffer.h>
#include <vst3_host.h>

static const float AUDIO_PEAK_OFFSET = 0.0000000001f;
static const float AUDIO_MIN_PEAK_DB = -200.0f;
static const int BUFFER_FRAME_SIZE = 512;
static const int CIRCULAR_BUFFER_SIZE = BUFFER_FRAME_SIZE * 2 + 10;

namespace godot {

class Vst3Instance : public Object {
    GDCLASS(Vst3Instance, Object);
    friend class Vst3Server;

private:
    uint64_t last_mix_time;
    int last_mix_frames;
    bool active;
    bool channels_cleared;

    int sfont_id;
    Vst3Host *vst3_host;
    bool finished;
    String instance_name;
    bool solo;
    bool mute;
    bool bypass;
    float volume_db;
    String uri;
    bool initialized;
    bool has_processed_audio;
    double mix_rate;

    bool thread_exited;
    mutable bool exit_thread;
    Ref<Thread> thread;
    Ref<Mutex> mutex;
    Ref<Semaphore> semaphore;

    struct Channel {
        String name;
        bool used = false;
        bool active = false;
        float peak_volume = AUDIO_MIN_PEAK_DB;
        Vst3CircularBuffer<float> buffer;
        Channel() {
        }
    };

    void *midi_buffer;

    std::vector<Vst3CircularBuffer<float>> input_channels;
    std::vector<Channel> output_channels;

    Vector<float> temp_buffer;

    Channel output_left_channel;
    Channel output_right_channel;

    TypedArray<Vst3Parameter> input_parameters;
    TypedArray<Vst3Parameter> output_parameters;

    TypedArray<String> presets;

    void configure();

    Error start_thread();
    void stop_thread();
    void lock();
    void unlock();
    void cleanup_channels();

protected:
    static void _bind_methods();

public:
    Vst3Instance();
    ~Vst3Instance();

    void start();
    void stop();
    void finish();
    void reset();

    void program_select(int chan, int bank_num, int preset_num);

    void note_on(int midi_bus, int chan, int key, int vel);
    void note_off(int midi_bus, int chan, int key);
    void control_change(int midi_bus, int chan, int control, int value);

    void send_input_parameter_channel(int p_channel, float p_value);
    float get_input_parameter_channel(int p_channel);

    void send_output_parameter_channel(int p_channel, float p_value);
    float get_output_parameter_channel(int p_channel);

    // val value (0-16383 with 8192 being center)
    void pitch_bend(int chan, int val);

    int process_sample(AudioFrame *p_buffer, float p_rate, int p_frames);

    void set_channel_sample(AudioFrame *p_buffer, float p_rate, int p_frames, int left, int right);
    int get_channel_sample(AudioFrame *p_buffer, float p_rate, int p_frames, int left, int right);

    void set_instance_name(const String &name);
    const String &get_instance_name();

    void set_uri(const String &p_uri);

    int get_input_channel_count();
    int get_output_channel_count();

    int get_input_midi_count();
    int get_output_midi_count();

    TypedArray<Vst3Parameter> get_input_parameters();
    TypedArray<Vst3Parameter> get_output_parameters();

    TypedArray<String> get_presets();
    void load_preset(String p_preset);

    double get_time_since_last_mix();
    double get_time_to_next_mix();

    void thread_func();
    void initialize();

    void set_active(bool active);
    bool is_active();
};
} // namespace godot

#endif
