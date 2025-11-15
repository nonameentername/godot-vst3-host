#ifndef AUDIO_STREAM_VST3_CHANNEL_H
#define AUDIO_STREAM_VST3_CHANNEL_H

#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/godot.hpp>

namespace godot {

class AudioStreamVst3Channel : public AudioStream {
    GDCLASS(AudioStreamVst3Channel, AudioStream)

private:
    friend class AudioStreamPlaybackVst3Channel;
    String instance_name;
    int channel_left;
    int channel_right;

public:
    virtual String get_stream_name() const;
    virtual int process_sample(AudioFrame *p_buffer, float p_rate, int p_frames);
    virtual float get_length() const;
    AudioStreamVst3Channel();
    ~AudioStreamVst3Channel();
    virtual Ref<AudioStreamPlayback> _instantiate_playback() const override;
    void set_instance_name(const String &name);
    const String &get_instance_name() const;
    void set_channel_left(int p_channel_left);
    int get_channel_left();
    void set_channel_right(int p_channel_right);
    int get_channel_right();

    bool _set(const StringName &p_name, const Variant &p_value);
    bool _get(const StringName &p_name, Variant &r_ret) const;
    void _get_property_list(List<PropertyInfo> *p_list) const;

    void layout_changed();

protected:
    static void _bind_methods();
};
} // namespace godot

#endif
