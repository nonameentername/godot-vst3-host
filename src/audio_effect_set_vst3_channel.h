#ifndef AUDIO_EFFECT_SEND_VST3_CHANNEL_H
#define AUDIO_EFFECT_SEND_VST3_CHANNEL_H

#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_server.hpp>

namespace godot {

class AudioEffectSetVst3Channel;

class AudioEffectSetVst3ChannelInstance : public AudioEffectInstance {
    GDCLASS(AudioEffectSetVst3ChannelInstance, AudioEffectInstance);

private:
    friend class AudioEffectSetVst3Channel;
    Ref<AudioEffectSetVst3Channel> base;
    bool has_data = false;

public:
    virtual void _process(const void *src_buffer, AudioFrame *dst_buffer, int32_t frame_count) override;
    virtual bool _process_silence() const override;

protected:
    static void _bind_methods();
};

class AudioEffectSetVst3Channel : public AudioEffect {
    GDCLASS(AudioEffectSetVst3Channel, AudioEffect)
    friend class AudioEffectSetVst3ChannelInstance;

    String instance_name;
    int channel_left = 0;
    int channel_right = 1;
    bool forward_audio = true;

protected:
    static void _bind_methods();

public:
    AudioEffectSetVst3Channel();
    ~AudioEffectSetVst3Channel();

    virtual Ref<AudioEffectInstance> _instantiate() override;

    void set_instance_name(const String &name);
    const String &get_instance_name() const;

    void set_channel_left(int p_channel_left);
    int get_channel_left();

    void set_channel_right(int p_channel_right);
    int get_channel_right();

    void set_forward_audio(bool p_forward_audio);
    bool get_forward_audio();

    bool _set(const StringName &p_name, const Variant &p_value);
    bool _get(const StringName &p_name, Variant &r_ret) const;
    void _get_property_list(List<PropertyInfo> *p_list) const;

    void layout_changed();
};
} // namespace godot

#endif
