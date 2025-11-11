#ifndef VST3_CIRCULAR_BUFFER_H
#define VST3_CIRCULAR_BUFFER_H

namespace godot {

const int CIRCULAR_BUFFER_SIZE = 2048;

template <typename T> struct AudioRingBuffer {
    int write_index = 0;
    int read_index = 0;

    T buffer[CIRCULAR_BUFFER_SIZE];
};

template <typename T> class Vst3CircularBuffer {

private:
    AudioRingBuffer<T> *audio_buffer;

public:
    Vst3CircularBuffer();
    ~Vst3CircularBuffer();

    // TODO: rename to write_buffer or write
    void write_channel(const T *p_buffer, int p_frames);

    // TODO: rename to read_buffer or read
    int read_channel(T *p_buffer, int p_frames);

    void set_read_index(int p_index);
    void set_write_index(int p_index);
    int update_read_index(int p_frames);
};

} // namespace godot

#endif
