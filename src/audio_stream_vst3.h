#ifndef AUDIO_STREAM_VST3_H
#define AUDIO_STREAM_VST3_H

#include "vst3_instance.h"

#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_playback.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

class AudioStreamVst3 : public AudioStream {
    GDCLASS(AudioStreamVst3, AudioStream)

private:
    friend class AudioStreamPlaybackVst3;
    String vst3_name;
    bool active;

    Vst3Instance *get_instance();
    void on_layout_changed();
    void on_vst3_ready(String vst3_name);

public:
    virtual String get_stream_name() const;
    virtual float get_length() const;

    int process_sample(AudioFrame *p_buffer, float p_rate, int p_frames);

    AudioStreamVst3();
    ~AudioStreamVst3();

    virtual Ref<AudioStreamPlayback> _instantiate_playback() const override;
    void set_instance_name(const String &name);
    const String &get_instance_name() const;

    void set_active(bool active);
    bool is_active();

    bool _set(const StringName &p_name, const Variant &p_value);
    bool _get(const StringName &p_name, Variant &r_ret) const;
    void _get_property_list(List<PropertyInfo> *p_list) const;

protected:
    static void _bind_methods();
};
} // namespace godot

#endif
