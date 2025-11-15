#include "audio_stream_player_vst3_channel.h"
#include "audio_stream_vst3_channel.h"

using namespace godot;

AudioStreamPlaybackVst3Channel::AudioStreamPlaybackVst3Channel() : active(false) {
}

AudioStreamPlaybackVst3Channel::~AudioStreamPlaybackVst3Channel() {
}

void AudioStreamPlaybackVst3Channel::_stop() {
    active = false;
}

void AudioStreamPlaybackVst3Channel::_start(double p_from_pos) {
    active = true;
}

void AudioStreamPlaybackVst3Channel::_seek(double p_time) {
    if (p_time < 0) {
        p_time = 0;
    }
}

int AudioStreamPlaybackVst3Channel::_mix(AudioFrame *p_buffer, float p_rate, int p_frames) {
    ERR_FAIL_COND_V(!active, 0);
    if (!active) {
        return 0;
    }

    return base->process_sample(p_buffer, p_rate, p_frames);
}

int AudioStreamPlaybackVst3Channel::_get_loop_count() const {
    return 10;
}

double AudioStreamPlaybackVst3Channel::_get_playback_position() const {
    return 0;
}

float AudioStreamPlaybackVst3Channel::_get_length() const {
    return 0.0;
}

bool AudioStreamPlaybackVst3Channel::_is_playing() const {
    return active;
}

void AudioStreamPlaybackVst3Channel::_bind_methods() {
}
