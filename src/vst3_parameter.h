#ifndef VST3_PARAMETER_H
#define VST3_PARAMETER_H

#include "godot_cpp/variant/dictionary.hpp"
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

namespace godot {

class Vst3Parameter : public RefCounted {
    GDCLASS(Vst3Parameter, RefCounted);

private:
    //TODO: is the index needed?
    int index;
    uint32_t id;
    String title;
    String short_title;
    String units;
    int step_count;
    int default_normalized_value;
    int unit_id;
    bool can_automate;
    bool is_read_only;
    bool is_wrap_around;
    bool is_list;
    bool is_hidden;
    bool is_program_change;
    bool is_bypass;

protected:
    static void _bind_methods();

public:
    Vst3Parameter();
    ~Vst3Parameter();

    void set_index(int p_index);
    int get_index();

    void set_id(int p_id);
    int get_id();

    void set_title(String p_title);
    String get_title();

    void set_short_title(String p_short_title);
    String get_short_title();

    void set_units(String p_units);
    String get_units();

    void set_step_count(int p_step_count);
    int get_step_count();

    void set_default_normalized_value(int p_default_normalized_value);
    int get_default_normalized_value();

    void set_unit_id(int p_unit_id);
    int get_unit_id();

    void set_can_automate(bool p_can_automate);
    bool get_can_automate();

    //TODO: maybe rename these get_is_ methods to get or is?
    void set_is_read_only(bool p_is_read_only);
    bool get_is_read_only();

    void set_is_wrap_around(bool p_is_wrap_around);
    bool get_is_wrap_around();

    void set_is_list(bool p_is_list);
    bool get_is_list();

    void set_is_hidden(bool p_is_hidden);
    bool get_is_hidden();

    void set_is_program_change(bool p_is_program_change);
    bool get_is_program_change();

    void set_is_bypass(bool p_is_bypass);
    bool get_is_bypass();
};

} // namespace godot
#endif
