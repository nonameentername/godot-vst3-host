#include "register_types.h"

#include <gdextension_interface.h>

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "audio_effect_get_vst3_channel.h"
#include "audio_effect_set_vst3_channel.h"
#include "audio_stream_player_vst3.h"
#include "audio_stream_player_vst3_channel.h"
#include "audio_stream_vst3.h"
#include "audio_stream_vst3_channel.h"
#include "editor_audio_meter_notches_vst3.h"
#include "vst3_parameter.h"
#include "vst3_instance.h"
#include "vst3_layout.h"
#include "vst3_server.h"
#include "vst3_server_node.h"

namespace godot {

static Vst3Server *vst3_server;

void initialize_godot_vst3_host_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<EditorAudioMeterNotchesVst3>();
    ClassDB::register_class<AudioStreamVst3>();
    ClassDB::register_class<AudioStreamPlaybackVst3>();
    ClassDB::register_class<AudioStreamVst3Channel>();
    ClassDB::register_class<AudioStreamPlaybackVst3Channel>();
    ClassDB::register_class<AudioEffectSetVst3Channel>();
    ClassDB::register_class<AudioEffectSetVst3ChannelInstance>();
    ClassDB::register_class<AudioEffectGetVst3Channel>();
    ClassDB::register_class<AudioEffectGetVst3ChannelInstance>();
    ClassDB::register_class<Vst3Parameter>();
    ClassDB::register_class<Vst3Layout>();
    ClassDB::register_class<Vst3ServerNode>();
    ClassDB::register_class<Vst3Instance>();
    ClassDB::register_class<Vst3Server>();

    vst3_server = memnew(Vst3Server);
    Engine::get_singleton()->register_singleton("Vst3Server", Vst3Server::get_singleton());
}

void uninitialize_godot_vst3_host_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    Engine::get_singleton()->unregister_singleton("Vst3Server");
    vst3_server->finish();
    memdelete(vst3_server);
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT godot_vst3_host_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                        const GDExtensionClassLibraryPtr p_library,
                                                        GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_godot_vst3_host_module);
    init_obj.register_terminator(uninitialize_godot_vst3_host_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
} // namespace godot
