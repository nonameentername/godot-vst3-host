#ifndef AUDIO_EFFECT_GET_VST3_CHANNEL_H
#define AUDIO_EFFECT_GET_VST3_CHANNEL_H

#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_server.hpp>

namespace godot {

class AudioEffectGetVst3Channel;

class AudioEffectGetVst3ChannelInstance : public AudioEffectInstance {
    GDCLASS(AudioEffectGetVst3ChannelInstance, AudioEffectInstance);

private:
    Vector<AudioFrame> temp_buffer;
    friend class AudioEffectGetVst3Channel;
    Ref<AudioEffectGetVst3Channel> base;
    bool has_data = false;

public:
    virtual void _process(const void *src_buffer, AudioFrame *dst_buffer, int32_t frame_count) override;
    virtual bool _process_silence() const override;

protected:
    static void _bind_methods();
};

class AudioEffectGetVst3Channel : public AudioEffect {
    GDCLASS(AudioEffectGetVst3Channel, AudioEffect)
    friend class AudioEffectGetVst3ChannelInstance;

    String instance_name;
    int channel_left = 0;
    int channel_right = 1;
    float mix = 1;

protected:
    static void _bind_methods();

public:
    AudioEffectGetVst3Channel();
    ~AudioEffectGetVst3Channel();

    virtual Ref<AudioEffectInstance> _instantiate() override;

    void set_instance_name(const String &name);
    const String &get_instance_name() const;

    void set_channel_left(int p_channel_left);
    int get_channel_left();

    void set_channel_right(int p_channel_right);
    int get_channel_right();

    void set_mix(float p_mix);
    float get_mix();

    bool _set(const StringName &p_name, const Variant &p_value);
    bool _get(const StringName &p_name, Variant &r_ret) const;
    void _get_property_list(List<PropertyInfo> *p_list) const;

    void layout_changed();
};
} // namespace godot

#endif
