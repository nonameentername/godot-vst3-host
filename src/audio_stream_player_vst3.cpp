#include "audio_stream_player_vst3.h"
#include "audio_stream_vst3.h"

using namespace godot;

AudioStreamPlaybackVst3::AudioStreamPlaybackVst3() : active(false) {
}

AudioStreamPlaybackVst3::~AudioStreamPlaybackVst3() {
    _stop();
}

void AudioStreamPlaybackVst3::_stop() {
    active = false;
    base->set_active(active);
}

void AudioStreamPlaybackVst3::_start(double p_from_pos) {
    active = true;
    base->set_active(active);
}

void AudioStreamPlaybackVst3::_seek(double p_time) {
    if (p_time < 0) {
        p_time = 0;
    }
}

int AudioStreamPlaybackVst3::_mix(AudioFrame *p_buffer, float p_rate, int p_frames) {
    ERR_FAIL_COND_V(!active, 0);
    if (!active) {
        return 0;
    }

    return base->process_sample(p_buffer, p_rate, p_frames);
}

void AudioStreamPlaybackVst3::_tag_used_streams() {
}

int AudioStreamPlaybackVst3::_get_loop_count() const {
    return 10;
}

double AudioStreamPlaybackVst3::_get_playback_position() const {
    return 0;
}

float AudioStreamPlaybackVst3::_get_length() const {
    return 0.0;
}

bool AudioStreamPlaybackVst3::_is_playing() const {
    return active;
}

void AudioStreamPlaybackVst3::_bind_methods() {
}
