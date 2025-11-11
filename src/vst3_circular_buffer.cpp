#include "vst3_circular_buffer.h"

using namespace godot;

template <typename T> Vst3CircularBuffer<T>::Vst3CircularBuffer() {
    audio_buffer = new AudioRingBuffer<T>();
}

template <typename T> Vst3CircularBuffer<T>::~Vst3CircularBuffer() {
    delete audio_buffer;
}

template <typename T> void Vst3CircularBuffer<T>::write_channel(const T *p_buffer, int p_frames) {
    for (int frame = 0; frame < p_frames; frame++) {
        audio_buffer->buffer[(audio_buffer->write_index + frame) % CIRCULAR_BUFFER_SIZE] = p_buffer[frame];
    }
    audio_buffer->write_index = (audio_buffer->write_index + p_frames) % CIRCULAR_BUFFER_SIZE;
}

template <typename T> int Vst3CircularBuffer<T>::read_channel(T *p_buffer, int p_frames) {
    const int read_index = audio_buffer->read_index;
    const int write_index = audio_buffer->write_index;

    int available = 0;

    if (write_index >= read_index) {
        available = write_index - read_index;
    } else {
        available = (CIRCULAR_BUFFER_SIZE - read_index) + write_index;
    }

    if (available < p_frames) {
        return 0;
    }

    for (int frame = 0; frame < p_frames; frame++) {
        p_buffer[frame] = audio_buffer->buffer[(read_index + frame) % CIRCULAR_BUFFER_SIZE];
    }

    return p_frames;
}

template <typename T> void Vst3CircularBuffer<T>::set_read_index(int p_index) {
    audio_buffer->read_index = p_index;
}

template <typename T> void Vst3CircularBuffer<T>::set_write_index(int p_index) {
    audio_buffer->write_index = p_index;
}

template <typename T> int Vst3CircularBuffer<T>::update_read_index(int p_frames) {
    const int read_index = audio_buffer->read_index;
    const int write_index = audio_buffer->write_index;

    // TODO: remove duplicate code
    int available = 0;

    if (write_index >= read_index) {
        available = write_index - read_index;
    } else {
        available = (CIRCULAR_BUFFER_SIZE - read_index) + write_index;
    }

    if (available < p_frames) {
        return 0;
    }

    audio_buffer->read_index = (read_index + p_frames) % CIRCULAR_BUFFER_SIZE;

    return p_frames;
}

namespace godot {
template class Vst3CircularBuffer<float>;
template class Vst3CircularBuffer<int>;
} // namespace godot
