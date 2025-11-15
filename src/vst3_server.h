#ifndef VST3_SERVER_H
#define VST3_SERVER_H

#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "vst3_instance.h"
#include "vst3_layout.h"

namespace godot {

class Vst3Server : public Object {
    GDCLASS(Vst3Server, Object);

private:
    bool initialized;
    bool layout_loaded;
    bool edited;
    int sfont_id;
    String vst3_path;

    Vst3Host *vst3_host;
    HashMap<String, Vst3Instance *> instance_map;

    bool thread_exited;
    mutable bool exit_thread;
    Ref<Thread> thread;
    Ref<Mutex> mutex;

    void on_ready(String instance_name);
    void add_property(String name, String default_value, GDExtensionVariantType extension_type, PropertyHint hint);

protected:
    bool solo_mode;
    Vector<Vst3Instance *> instances;
    static void _bind_methods();
    static Vst3Server *singleton;

public:
    Vst3Server();
    ~Vst3Server();

    bool get_solo_mode();

    void set_edited(bool p_edited);
    bool get_edited();

    static Vst3Server *get_singleton();
    void initialize();
    void process();
    void thread_func();

    void set_instance_count(int p_count);
    int get_instance_count() const;

    void remove_instance(int p_index);
    void add_instance(int p_at_pos = -1);

    void move_instance(int p_index, int p_to_pos);

    void set_instance_name(int p_index, const String &p_name);
    String get_instance_name(int p_index) const;
    int get_instance_index(const StringName &p_name) const;

    String get_name_options() const;

    int get_channel_count(int p_index) const;

    void set_volume_db(int p_index, float p_volume_db);
    float get_volume_db(int p_index) const;

    void set_uri(int p_index, String p_uri);
    String get_uri(int p_index) const;

    void set_solo(int p_index, bool p_enable);
    bool is_solo(int p_index) const;

    void set_mute(int p_index, bool p_enable);
    bool is_mute(int p_index) const;

    void set_bypass(int p_index, bool p_enable);
    bool is_bypassing(int p_index) const;

    float get_channel_peak_volume_db(int p_index, int p_channel) const;

    bool is_channel_active(int p_index, int p_channel) const;

    bool load_default_layout();
    void set_layout(const Ref<Vst3Layout> &p_layout);
    Ref<Vst3Layout> generate_layout() const;

    Error start();
    void lock();
    void unlock();
    void finish();

    TypedArray<String> get_plugins();
    String get_plugin_name(String p_uri);

    Vst3Instance *get_instance(const String &p_name);
    Vst3Instance *get_instance_by_index(int p_index);
    Vst3Instance *get_instance_(const Variant &p_name);

    String get_version();
    String get_build();
};
} // namespace godot

#endif
