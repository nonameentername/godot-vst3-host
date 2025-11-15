#ifndef VST3_LAYOUT_H
#define VST3_LAYOUT_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

class Vst3Layout : public Resource {
    GDCLASS(Vst3Layout, Resource);
    friend class Vst3Server;

    struct Vst3 {
        String name;
        bool solo = false;
        bool mute = false;
        bool bypass = false;
        float volume_db = 0.0f;
        String uri;

        Vst3() {
        }
    };

    Vector<Vst3> instances;

protected:
    static void _bind_methods();

    bool _set(const StringName &p_name, const Variant &p_value);
    bool _get(const StringName &p_name, Variant &r_ret) const;
    void _get_property_list(List<PropertyInfo> *p_list) const;

public:
    Vst3Layout();
};

} // namespace godot

#endif
