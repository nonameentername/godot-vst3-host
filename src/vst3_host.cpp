#include "vst3_host.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <memory_resource>
#include <string>
#include <vector>
#include <filesystem>

// --- VST3 SDK (headers provided by vcpkg's vst3sdk port)
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/utility/memoryibstream.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"

namespace {
using namespace Steinberg;
using namespace Steinberg::Vst;
// using namespace VST3;
// using namespace VST3::Hosting;

template <class T> struct FDeleter {
    void operator()(T *p) const {
        if (p) {
            p->release();
        }
    }
};

template <class T> using FUPtr = std::unique_ptr<T, FDeleter<T>>;

struct SimpleIBStream : Steinberg::IBStream {
    std::vector<char> buf;
    Steinberg::int64 pos{0};

    explicit SimpleIBStream(size_t reserve = 0) {
        buf.reserve(reserve);
    }

    // IStream methods
    Steinberg::tresult PLUGIN_API read(void *data, Steinberg::int32 numBytes,
                                       Steinberg::int32 *numBytesRead) SMTG_OVERRIDE {
        if (pos < 0) {
            pos = 0;
        }
        Steinberg::int64 avail = (Steinberg::int64)buf.size() - pos;
        Steinberg::int32 toRead =
            (Steinberg::int32)std::max<Steinberg::int64>(0, std::min<Steinberg::int64>(avail, numBytes));
        if (toRead > 0) {
            std::memcpy(data, buf.data() + pos, (size_t)toRead);
        }
        pos += toRead;
        if (numBytesRead) {
            *numBytesRead = toRead;
        }
        return (toRead == numBytes) ? Steinberg::kResultOk : Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API write(void *data, Steinberg::int32 numBytes,
                                        Steinberg::int32 *numBytesWritten) SMTG_OVERRIDE {
        if (pos < 0) {
            pos = 0;
        }
        const Steinberg::int64 endPos = pos + numBytes;
        if (endPos > (Steinberg::int64)buf.size()) {
            buf.resize((size_t)endPos);
        }
        std::memcpy(buf.data() + pos, data, (size_t)numBytes);
        pos = endPos;
        if (numBytesWritten) {
            *numBytesWritten = numBytes;
        }
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API seek(Steinberg::int64 pos_, Steinberg::int32 mode,
                                       Steinberg::int64 *resultPos) SMTG_OVERRIDE {
        Steinberg::int64 newPos = pos;
        if (mode == Steinberg::IBStream::kIBSeekSet) {
            newPos = pos_;
        } else if (mode == Steinberg::IBStream::kIBSeekCur) {
            newPos = pos + pos_;
        } else if (mode == Steinberg::IBStream::kIBSeekEnd) {
            newPos = (Steinberg::int64)buf.size() + pos_;
        }
        newPos = std::max<Steinberg::int64>(0, newPos);
        pos = newPos;
        if (resultPos) {
            *resultPos = pos;
        }
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API tell(Steinberg::int64 *pPos) SMTG_OVERRIDE {
        if (pPos) {
            *pPos = pos;
        }
        return Steinberg::kResultOk;
    }

    // IUnknown
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID, void **) SMTG_OVERRIDE {
        return Steinberg::kNoInterface;
    }
    Steinberg::uint32 PLUGIN_API addRef() SMTG_OVERRIDE {
        return 1;
    }
    Steinberg::uint32 PLUGIN_API release() SMTG_OVERRIDE {
        return 1;
    }
};

struct SimpleEventList : Steinberg::Vst::IEventList {
    std::vector<Steinberg::Vst::Event> evs;

    Steinberg::int32 PLUGIN_API getEventCount() SMTG_OVERRIDE {
        return (Steinberg::int32)evs.size();
    }
    Steinberg::tresult PLUGIN_API getEvent(Steinberg::int32 index, Steinberg::Vst::Event &e) SMTG_OVERRIDE {
        if (index < 0 || index >= (Steinberg::int32)evs.size()) {
            return Steinberg::kResultFalse;
        }
        e = evs[(size_t)index];
        return Steinberg::kResultOk;
    }
    Steinberg::tresult PLUGIN_API addEvent(Steinberg::Vst::Event &e) SMTG_OVERRIDE {
        evs.push_back(e);
        return Steinberg::kResultOk;
    }
    // IUnknown plumbing
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID, void **) SMTG_OVERRIDE {
        return Steinberg::kNoInterface;
    }
    Steinberg::uint32 PLUGIN_API addRef() SMTG_OVERRIDE {
        return 1;
    }
    Steinberg::uint32 PLUGIN_API release() SMTG_OVERRIDE {
        return 1;
    }
};

struct SimpleParamValueQueue : Steinberg::Vst::IParamValueQueue {
    Steinberg::Vst::ParamID id{};
    struct Pt {
        Steinberg::int32 off;
        Steinberg::Vst::ParamValue v;
    };
    std::vector<Pt> pts;
    Steinberg::Vst::ParamID PLUGIN_API getParameterId() SMTG_OVERRIDE {
        return id;
    }
    Steinberg::int32 PLUGIN_API getPointCount() SMTG_OVERRIDE {
        return (Steinberg::int32)pts.size();
    }
    Steinberg::tresult PLUGIN_API getPoint(Steinberg::int32 i, Steinberg::int32 &off,
                                           Steinberg::Vst::ParamValue &v) SMTG_OVERRIDE {
        if (i < 0 || i >= (Steinberg::int32)pts.size()) {
            return Steinberg::kResultFalse;
        }
        off = pts[i].off;
        v = pts[i].v;
        return Steinberg::kResultOk;
    }
    Steinberg::tresult PLUGIN_API addPoint(Steinberg::int32 off, Steinberg::Vst::ParamValue v,
                                           Steinberg::int32 &index) SMTG_OVERRIDE {
        index = (Steinberg::int32)pts.size();
        pts.push_back({off, v});
        return Steinberg::kResultOk;
    }
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID, void **) SMTG_OVERRIDE {
        return Steinberg::kNoInterface;
    }
    Steinberg::uint32 PLUGIN_API addRef() SMTG_OVERRIDE {
        return 1;
    }
    Steinberg::uint32 PLUGIN_API release() SMTG_OVERRIDE {
        return 1;
    }
};

struct SimpleParameterChanges : Steinberg::Vst::IParameterChanges {
    std::vector<std::unique_ptr<SimpleParamValueQueue>> qs;
    Steinberg::int32 PLUGIN_API getParameterCount() SMTG_OVERRIDE {
        return (Steinberg::int32)qs.size();
    }
    Steinberg::Vst::IParamValueQueue *PLUGIN_API getParameterData(Steinberg::int32 i) SMTG_OVERRIDE {
        if (i < 0 || i >= (Steinberg::int32)qs.size()) {
            return nullptr;
        }
        return qs[i].get();
    }
    Steinberg::Vst::IParamValueQueue *PLUGIN_API addParameterData(const Steinberg::Vst::ParamID &id,
                                                                  Steinberg::int32 &index) SMTG_OVERRIDE {
        for (size_t i = 0; i < qs.size(); ++i) {
            if (qs[i]->id == id) {
                index = (Steinberg::int32)i;
                return qs[i].get();
            }
        }
        qs.push_back(std::make_unique<SimpleParamValueQueue>());
        qs.back()->id = id;
        index = (Steinberg::int32)qs.size() - 1;
        return qs.back().get();
    }
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID, void **) SMTG_OVERRIDE {
        return Steinberg::kNoInterface;
    }
    Steinberg::uint32 PLUGIN_API addRef() SMTG_OVERRIDE {
        return 1;
    }
    Steinberg::uint32 PLUGIN_API release() SMTG_OVERRIDE {
        return 1;
    }
};

} // namespace

// --------------------------------------------------------------------------------------
// godot::Vst3Host implementation
// --------------------------------------------------------------------------------------
using namespace godot;

Vst3Host::Vst3Host(double p_sr, int p_frames, uint32_t seq_bytes) {
    seq_capacity_hint = seq_bytes;
    // Pre-size minimal stereo I/O buffers the way your API expects
    int channels = 0;

    audio.resize(channels);
    audio_ptrs.resize(channels);
    audio_in_ptrs.resize(channels);
    audio_out_ptrs.resize(channels);

    for (uint32_t ch = 0; ch < channels; ++ch) {
        audio[ch].assign(p_frames, 0.0f);
        audio_ptrs[ch] = audio[ch].data();
        audio_in_ptrs[ch] = audio[ch].data();
        audio_out_ptrs[ch] = audio[ch].data();
    }

    midi_input_buffer.clear();
    midi_output_buffer.clear();

    state.sample_rate = p_sr;
    state.block_size = p_frames;

    default_search_paths = {
#if defined(_WIN32)
        "C:/Program Files/Common Files/VST3/"
#elif defined(__APPLE__)
            "/Library/Audio/Plug-Ins/VST3/",
        getenv("HOME") + "/Library/Audio/Plug-Ins/VST3/"
#else
            "/usr/lib/vst3/",
        std::string(getenv("HOME")) + "/.vst3/"
#endif
    };
}

Vst3Host::~Vst3Host() {
	clear_plugin();
}

std::vector<std::string> Vst3Host::get_vst3_recursive(const std::vector<std::string>& paths) {
	std::vector<std::string> results;

	for (const auto& root : paths) {
		std::filesystem::path p(root);

		if (!std::filesystem::exists(p))
			continue;

		for (auto& entry : std::filesystem::recursive_directory_iterator(p)) {
			if (entry.is_directory() && entry.path().extension() == ".vst3") {
				results.push_back(entry.path().string());
			}
		}
	}

	return results;
}

std::vector<Vst3PluginInfo> Vst3Host::get_plugins_info() {
    std::vector<Vst3PluginInfo> result;

	std::vector<std::string> plugin_paths = get_vst3_recursive(default_search_paths);

	for (auto& plugin_path : plugin_paths) {
		std::string error;
		VST3::Hosting::Module::Ptr plugin_module = VST3::Hosting::Module::create(plugin_path, error);

		if (!plugin_module) {
			std::fprintf(stderr, "VST3: failed to load module: %s\n%s", plugin_path.c_str(), error.c_str());
			continue;
		}

		auto factory = plugin_module->getFactory();

		for (const auto &ci : factory.classInfos()) {
			if (ci.category() == kVstAudioEffectClass) {
				Vst3PluginInfo info;
				//info.uri = ci.ID().toString(); // not a real URI; just stable ID string
				info.uri = plugin_path; // not a real URI; just stable ID string
				info.name = ci.name().c_str();
				result.push_back(std::move(info));
			}
		}
	}

    return result;
}

void Vst3Host::clear_plugin() {
    if (state.processor && state.processing) {
        state.processor->setProcessing(false);
        state.processing = false;
    }
    if (state.component && state.active) {
        state.component->setActive(false);
        state.active = false;
    }

    if (state.component && state.controller) {
        FUnknownPtr<IConnectionPoint> cp1(state.component);
        FUnknownPtr<IConnectionPoint> cp2(state.controller);
        if (cp1 && cp2) {
            cp1->disconnect(cp2);
            cp2->disconnect(cp1);
        }
    }

    if (state.controller) {
        state.controller->terminate();
        state.controller = nullptr;
    }

    state.processor = nullptr;

    if (state.component) {
        state.component->terminate();
        state.component = nullptr;
    }

    if (state.plug_provider) {
        state.plug_provider->release();
    }

    if (state.module) {
        state.module.reset();
    }

    audio_in_map.clear();
    audio_out_map.clear();
    midi_in_map.clear();
    midi_out_map.clear();

    midi_input_buffer.clear();
    midi_output_buffer.clear();

    audio.clear();
    audio_ptrs.clear();
    audio_in_ptrs.clear();
    audio_out_ptrs.clear();

    parameter_input_buffer.clear();
    parameter_output_buffer.clear();
    parameter_inputs.clear();
    parameter_outputs.clear();

    state.has_class = false;
}

bool Vst3Host::find_plugin(const std::string &p_plugin_path) {
    plugin_path = p_plugin_path;

    const std::string &path = plugin_path;

    std::string error;

    VST3::Hosting::Module::Ptr plugin_module = VST3::Hosting::Module::create(plugin_path, error);
    if (!plugin_module) {
        std::fprintf(stderr, "VST3: failed to load module: %s\n%s", plugin_path.c_str(), error.c_str());
        return false;
    }

    auto factory = plugin_module->getFactory(); // value, not pointer
    bool has_class = false;

    for (const auto &ci : factory.classInfos()) {
        if (ci.category() == kVstAudioEffectClass) {
            has_class = true;
            break;
        }
    }

    if (!has_class) {
        std::fprintf(stderr, "VST3: no audio effect class exported by %s\n", plugin_path.c_str());
        return false;
    }
    return true;
}

bool Vst3Host::instantiate() {
	clear_plugin();

    std::string error;
    state.module = VST3::Hosting::Module::create(plugin_path, error);
    if (!state.module) {
        std::fprintf(stderr, "VST3: failed to load module: %s\n%s", plugin_path.c_str(), error.c_str());
        return false;
    }

    auto factory = state.module->getFactory(); // value, not pointer
    factory.setHostContext(&state.host_app);   // optional but good

    state.has_class = false;
    for (const auto &ci : factory.classInfos()) {
        if (ci.category() == kVstAudioEffectClass) {
            state.chosen_class = ci; // copy by value
            state.has_class = true;
            break;
        }
    }

    state.plug_provider = new PlugProvider (factory, state.chosen_class, true);

    if(!state.plug_provider->initialize()) {
        std::fprintf(stderr, "VST3: could not initialize plugin exported by %s\n", plugin_path.c_str());
        return false;
    }

    state.component = state.plug_provider->getComponentPtr();
    state.controller = state.plug_provider->getControllerPtr();

    // --- 2) Grab IAudioProcessor from the same object
    state.processor = Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor>(state.component);
    if (!state.processor) {
        std::fprintf(stderr, "VST3: component has no IAudioProcessor\n");
        return false;
    }

    // --- 5) Sync controller with component state (optional but common)
    ResizableMemoryIBStream memory_state;
    if (state.component->getState(&memory_state) == Steinberg::kResultOk) {
        memory_state.seek(0, Steinberg::IBStream::kIBSeekSet, nullptr);
        if (state.controller) {
            state.controller->setComponentState(&memory_state);
        }
    }

    // Connect component and controller
    if (state.component && state.controller) {
        FUnknownPtr<IConnectionPoint> cp1(state.component);
        FUnknownPtr<IConnectionPoint> cp2(state.controller);
        if (cp1 && cp2) {
            cp1->connect(cp2);
            cp2->connect(cp1);
        }
    }

    // --- 6) Activate buses the plugin marks default-active
    auto activate_all = [&](Steinberg::Vst::MediaTypes mt, Steinberg::Vst::BusDirections dir) {
        const Steinberg::int32 n = state.component->getBusCount(mt, dir);
        for (Steinberg::int32 i = 0; i < n; ++i) {
            Steinberg::Vst::BusInfo bi{};
            if (state.component->getBusInfo(mt, dir, i, bi) == Steinberg::kResultOk) {
                if ((bi.flags & Steinberg::Vst::BusInfo::kDefaultActive) != 0) {
                    state.component->activateBus(mt, dir, i, true);
                }
            }
        }
    };
    activate_all(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kInput);
    activate_all(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kOutput);
    activate_all(Steinberg::Vst::MediaTypes::kEvent, Steinberg::Vst::BusDirections::kInput);
    activate_all(Steinberg::Vst::MediaTypes::kEvent, Steinberg::Vst::BusDirections::kOutput);

    // --- 7) Set bus arrangements
    std::vector<Steinberg::Vst::SpeakerArrangement> inArrs, outArrs;
    auto query_arrs = [&](Steinberg::Vst::MediaTypes mt, Steinberg::Vst::BusDirections dir,
                          std::vector<Steinberg::Vst::SpeakerArrangement> &arrs) {
        const Steinberg::int32 n = state.component->getBusCount(mt, dir);
        arrs.resize(std::max<Steinberg::int32>(n, 0));
        for (Steinberg::int32 i = 0; i < n; ++i) {
            Steinberg::Vst::SpeakerArrangement a{};
            if (state.processor->getBusArrangement(dir, i, a) != Steinberg::kResultOk) {
                Steinberg::Vst::BusInfo bi{};
                state.component->getBusInfo(mt, dir, i, bi);
                a = (bi.channelCount == 1) ? Steinberg::Vst::SpeakerArr::kMono : Steinberg::Vst::SpeakerArr::kStereo;
            }
            arrs[i] = a;
        }
    };
    query_arrs(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kInput, inArrs);
    query_arrs(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kOutput, outArrs);

    auto *inPtr = inArrs.empty() ? nullptr : inArrs.data();
    auto *outPtr = outArrs.empty() ? nullptr : outArrs.data();
    state.processor->setBusArrangements(inPtr, (Steinberg::int32)inArrs.size(), outPtr, (Steinberg::int32)outArrs.size());

    // --- 8) Setup processing
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock = state.block_size;
    setup.sampleRate = state.sample_rate;
    if (state.processor->setupProcessing(setup) != Steinberg::kResultOk) {
        std::fprintf(stderr, "VST3: setupProcessing failed\n");
        return false;
    }

    // --- 9) (Optional) Activate now so some plugins finalize parameters
    state.component->setActive(true);
    state.active = true;
    state.processor->setProcessing(true);
    state.processing = true;

    // --- 10) Build parameter cache
    parameter_input_buffer.clear();
    parameter_output_buffer.clear();

    parameter_inputs.clear();
    parameter_outputs.clear();

    auto get_string = [&](const UString128 &p_value) {
        char buffer[256] = {0};
        p_value.toAscii(buffer, sizeof(buffer));
        return std::string(buffer);
    };

    if (state.controller) {
        const auto count = state.controller->getParameterCount();
        //       std::fprintf(stderr, "VST3: controller param count = %d\n", (int)count);

        for (Steinberg::int32 i = 0; i < count; ++i) {
            Steinberg::Vst::ParameterInfo pi{};
            if (state.controller->getParameterInfo(i, pi) != Steinberg::kResultOk) {
                continue;
            }

            if (pi.flags & ParameterInfo::kIsHidden) {
                continue;
            }

            Vst3ParameterBuffer p;
            p.id = pi.id;
            p.value = state.controller->getParamNormalized(pi.id);
            p.dirty = false;

            if (pi.flags & ParameterInfo::kIsReadOnly) {
                parameter_output_buffer.push_back(std::move(p));
            } else {
                parameter_input_buffer.push_back(std::move(p));
            }

            host::Vst3Parameter parameter;
            parameter.id = pi.id;
            parameter.title = get_string(pi.title);
            parameter.short_title = get_string(pi.shortTitle);
            parameter.units = get_string(pi.units);
            parameter.step_count = pi.stepCount;
            parameter.default_normalized_value = pi.defaultNormalizedValue;
            parameter.unit_id = pi.unitId;
            parameter.can_automate = pi.flags & ParameterInfo::kCanAutomate;
            parameter.is_read_only = pi.flags & ParameterInfo::kIsReadOnly;
            parameter.is_wrap_around = pi.flags & ParameterInfo::kIsWrapAround;
            parameter.is_list = pi.flags & ParameterInfo::kIsList;
            parameter.is_hidden = pi.flags & ParameterInfo::kIsHidden;
            parameter.is_program_change = pi.flags & ParameterInfo::kIsProgramChange;
            parameter.is_bypass = pi.flags & ParameterInfo::kIsBypass;

            if (pi.flags & ParameterInfo::kIsReadOnly) {
                parameter_outputs.push_back(std::move(parameter));
            } else {
                parameter_inputs.push_back(std::move(parameter));
            }
        }
    }

    // std::fprintf(stderr, "VST3: parameter_input_buffer.size() = %d\n", (int)parameter_input_buffer.size());
    // std::fprintf(stderr, "VST3: parameter_output_buffer.size() = %d\n", (int)parameter_output_buffer.size());

    return true;
}

void Vst3Host::wire_worker_interface() {
    // No-op in this minimal host
}

void Vst3Host::set_cli_parameter_overrides(const std::vector<std::pair<std::string, float>> &nvp) {
    cli_sets = nvp;
}

bool Vst3Host::prepare_ports_and_buffers(int p_frames) {
    int input_midi_channels = 0;
    int output_midi_channels = 0;

    for (int bus = 0; bus < state.component->getBusCount(MediaTypes::kEvent, BusDirections::kInput); bus++) {
        BusInfo info{};
        state.component->getBusInfo(MediaTypes::kEvent, BusDirections::kInput, bus, info);

        for (int channel = 0; channel < info.channelCount; channel++) {
            input_midi_channels++;
            ChannelMap channel_map;
            channel_map.bus = bus;
            channel_map.channel = channel;
            midi_in_map.push_back(channel_map);
        }
    }

    for (int bus = 0; bus < state.component->getBusCount(MediaTypes::kEvent, BusDirections::kOutput); bus++) {
        BusInfo info{};
        state.component->getBusInfo(MediaTypes::kEvent, BusDirections::kOutput, bus, info);

        for (int channel = 0; channel < info.channelCount; channel++) {
            output_midi_channels++;
            ChannelMap channel_map;
            channel_map.bus = bus;
            channel_map.channel = channel;
            midi_out_map.push_back(channel_map);
        }
    }

    midi_input_buffer.resize(input_midi_channels);
    midi_output_buffer.resize(output_midi_channels);

    int num_audio_bus_in = state.component->getBusCount(MediaTypes::kAudio, BusDirections::kInput);
    int num_audio_bus_out = state.component->getBusCount(MediaTypes::kAudio, BusDirections::kOutput);

    int channels = 0;
    int input_channels = 0;
    int output_channels = 0;

    for (int32 bus = 0; bus < num_audio_bus_in; bus++) {
        BusInfo info{};
        state.component->getBusInfo(MediaTypes::kAudio, BusDirections::kInput, bus, info);

        for (int channel = 0; channel < info.channelCount; channel++) {
            channels = channels + 1;
            input_channels = input_channels + 1;
            ChannelMap channel_map;
            channel_map.bus = bus;
            channel_map.channel = channel;
            audio_in_map.push_back(channel_map);
        }
    }

    for (int32 bus = 0; bus < num_audio_bus_out; bus++) {
        BusInfo info{};
        state.component->getBusInfo(MediaTypes::kAudio, BusDirections::kOutput, bus, info);

        for (int channel = 0; channel < info.channelCount; channel++) {
            channels = channels + 1;
            output_channels = output_channels + 1;
            ChannelMap channel_map;
            channel_map.bus = bus;
            channel_map.channel = channel;
            audio_out_map.push_back(channel_map);
        }
    }

    audio.resize(channels);
    audio_ptrs.resize(channels);
    audio_in_ptrs.resize(input_channels);
    audio_out_ptrs.resize(output_channels);

    for (uint32_t ch = 0; ch < channels; ++ch) {
        if ((int)audio[ch].size() < p_frames) {
            audio[ch].assign(p_frames, 0.0f);
        }
        audio_ptrs[ch] = audio[ch].data();
    }

    int channel_index = 0;

    for (int ch = 0; ch < input_channels; ch++) {
        audio_in_ptrs[ch] = audio[channel_index++].data();
    }

    for (int ch = 0; ch < output_channels; ch++) {
        audio_out_ptrs[ch] = audio[channel_index++].data();
    }

    return true;
}

void Vst3Host::activate() {
    if (!state.component || !state.processor || state.active) {
        return;
    }

    state.component->setActive(true);
    state.active = true;
    state.processor->setProcessing(true);
    state.processing = true;
}

void Vst3Host::deactivate() {
    if (!state.component || !state.processor) {
        return;
    }
    if (state.processing) {
        state.processor->setProcessing(false);
        state.processing = false;
    }
    if (state.active) {
        state.component->setActive(false);
        state.active = false;
    }
}

std::vector<std::string> Vst3Host::get_presets() {
    std::vector<std::string> result;

    /*
        FUnknownPtr<Steinberg::Vst::IUnitInfo> unit_info(state.controller);   // controller from your instance
        if (unit_info) {
            const int32 listCount = unit_info->getProgramListCount();
            for (int32 li = 0; li < listCount; ++li) {
                ProgramListInfo pli{};
                if (unit_info->getProgramListInfo(li, pli) == kResultOk) {
                    // pli.id  (ProgramListID)
                    // pli.name (String128)
                    // pli.programCount
                    for (int32 pi = 0; pi < pli.programCount; ++pi) {
                        String128 state{};
                        if (unit_info->getProgramName(pli.id, pi, state) == kResultOk) {
                            char name[256]{}; UString128(state).toAscii(name, sizeof(name));
                            //result.push_back(name);
                            // -> collect (pli.id, pi, name)
                        }
                    }
                }
            }
        }
    */

    return result;
}

void Vst3Host::load_preset(std::string preset) {

    /*
    ResizableMemoryIBStream state;
    state.component->getState(&state);

    char data[1024];

    int read;
    state.seek(0, IBStream::kIBSeekSet, NULL);

    //TODO: actually think about how to save this.
    state.read(data, 1024, &read);

    while (read > 0) {
        state.read(data, 1024, &read);
    }
    */

    // TODO: seems like most plugins aren't using this
    /*

        FUnknownPtr<Steinberg::Vst::IUnitInfo> unit_info(state.controller);   // controller from your instance
        FUnknownPtr<Steinberg::Vst::IProgramListData> program_list_data(state.controller);

        if (unit_info && program_list_data) {
            const int32 listCount = unit_info->getProgramListCount();
            for (int32 li = 0; li < listCount; ++li) {
                ProgramListInfo pli{};
                if (unit_info->getProgramListInfo(li, pli) == kResultOk) {
                    // pli.id  (ProgramListID)
                    // pli.name (String128)
                    // pli.programCount
                    for (int32 pi = 0; pi < pli.programCount; ++pi) {
                        String128 state{};
                        if (unit_info->getProgramName(pli.id, pi, state) == kResultOk) {
                            char name[256]{}; UString128(state).toAscii(name, sizeof(name));

                            //TODO: use the name instead of using the first one
                            ResizableMemoryIBStream program_data;
                            program_list_data->getProgramData(pli.id, pi, &program_data);
                            unit_info->setUnitProgramData(pli.id, pi, &program_data);
                            break;
                        }
                    }
                }
            }
        }
    */
}

int Vst3Host::perform(int p_frames) {
    if (!state.processor || !state.processing) {
        return 0;
    }

    using namespace Steinberg;
    using namespace Steinberg::Vst;

    // --- Build audio bus buffers on the fly from your linear arrays ---
    const int inBusCount = state.component->getBusCount(MediaTypes::kAudio, BusDirections::kInput);
    const int outBusCount = state.component->getBusCount(MediaTypes::kAudio, BusDirections::kOutput);

    std::vector<AudioBusBuffers> inBuses(inBusCount);
    std::vector<AudioBusBuffers> outBuses(outBusCount);
    std::vector<std::vector<float *>> inChanPtrs(inBusCount);
    std::vector<std::vector<float *>> outChanPtrs(outBusCount);

    // We walk your linear arrays in order they were built in prepare_ports_and_buffers()
    int linInIdx = 0, linOutIdx = 0;

    for (int b = 0; b < inBusCount; ++b) {
        BusInfo bi{};
        state.component->getBusInfo(MediaTypes::kAudio, BusDirections::kInput, b, bi);
        const int C = bi.channelCount;
        inChanPtrs[b].resize(C);
        for (int ch = 0; ch < C; ++ch) {
            inChanPtrs[b][ch] = (linInIdx < (int)audio_in_ptrs.size()) ? audio_in_ptrs[linInIdx++] : nullptr;
        }
        inBuses[b].numChannels = C;
        inBuses[b].silenceFlags = 0;
        inBuses[b].channelBuffers32 = (C > 0) ? inChanPtrs[b].data() : nullptr;
    }

    for (int b = 0; b < outBusCount; ++b) {
        BusInfo bi{};
        state.component->getBusInfo(MediaTypes::kAudio, BusDirections::kOutput, b, bi);
        const int C = bi.channelCount;
        outChanPtrs[b].resize(C);
        for (int ch = 0; ch < C; ++ch) {
            outChanPtrs[b][ch] = (linOutIdx < (int)audio_out_ptrs.size()) ? audio_out_ptrs[linOutIdx++] : nullptr;
        }
        outBuses[b].numChannels = C;
        outBuses[b].silenceFlags = 0;
        outBuses[b].channelBuffers32 = (C > 0) ? outChanPtrs[b].data() : nullptr;
    }

    // --- Build MIDI/Event input list by draining your ring buffers ---
    SimpleEventList inEvents;
    inEvents.evs.clear();

    // If you have multiple event input buses, we’ll default all events to bus 0.
    // (Most plugins only expose one). If you later want true multi-bus, add
    // a 'bus' field to your ring items and set e.busIndex accordingly (if available).
    const int evtInBusCount = state.component->getBusCount(MediaTypes::kEvent, BusDirections::kInput);

    for (int linMidi = 0; linMidi < (int)midi_input_buffer.size(); ++linMidi) {
        MidiEvent me{};
        // Drain all events queued for this linear "MIDI channel"
        while (read_midi_in(linMidi, me)) {
            const uint8_t status = me.data[0] & 0xF0;
            const uint8_t chan = me.data[0] & 0x0F;
            Event e{}; // zero-init
            e.sampleOffset = std::clamp(me.frame, 0, std::max(0, p_frames - 1));

            if (status == 0x90 && me.data[2] != 0) {
                e.type = Event::kNoteOnEvent;
                e.noteOn.channel = chan;
                e.noteOn.pitch = me.data[1];
                e.noteOn.velocity = me.data[2] / 127.f;
            } else if (status == 0x80 || (status == 0x90 && me.data[2] == 0)) {
                e.type = Event::kNoteOffEvent;
                e.noteOff.channel = chan;
                e.noteOff.pitch = me.data[1];
                e.noteOff.velocity = me.data[2] / 127.f;
            } else if (status == 0xE0) {
                // Pitch bend (14-bit): data1 = LSB, data2 = MSB
                const int bend = (int)((me.data[2] << 7) | me.data[1]); // 0..16383
                // Map to [-1,1] if you later implement kPolyPressure/kCtrlChange, etc.
                // For now, ignore or convert to controller if needed.
                continue;
            } else {
                // TODO: handle CC/aftertouch/PGM via IParameterChanges or kLegacyMIDICCOutEvent if you wish.
                continue;
            }

            // If your SDK version/struct exposes e.busIndex, you can set it:
            // e.busIndex = (evtInBusCount > 1) ? midi_in_map[linMidi].bus : 0;

            inEvents.addEvent(e);
        }
    }

    // Optional: collect plugin-emitted events (sequencers, arps)
    SimpleEventList outEvents;
    outEvents.evs.clear();

    // --- Fill ProcessData and process ---
    state.pd = {}; // reset
    state.pd.processMode = kRealtime;
    state.pd.symbolicSampleSize = kSample32;
    state.pd.numSamples = p_frames;

    state.pd.numInputs = (int32)inBuses.size();
    state.pd.numOutputs = (int32)outBuses.size();
    state.pd.inputs = inBuses.empty() ? nullptr : inBuses.data();
    state.pd.outputs = outBuses.empty() ? nullptr : outBuses.data();

    state.pd.inputParameterChanges = nullptr;
    state.pd.outputParameterChanges = nullptr;

    state.pd.inputEvents = (evtInBusCount > 0) ? &inEvents : nullptr;
    state.pd.outputEvents = &outEvents;

    SimpleParameterChanges paramChanges; // from earlier helper
    bool hasChanges = false;

    for (auto &p : parameter_input_buffer) {
        if (!p.dirty) {
            continue;
        }
        p.dirty = false;
        hasChanges = true;

        // Sync controller (optional but standard host behavior)
        if (state.controller) {
            state.controller->setParamNormalized(p.id, p.value);
        }

        // Tell processor: value at start of block (offset 0)
        Steinberg::int32 qi = 0, dummy = 0;
        if (auto *q = paramChanges.addParameterData(p.id, qi)) {
            q->addPoint(0, (Steinberg::Vst::ParamValue)p.value, dummy);
        }
    }

    state.pd.inputParameterChanges = hasChanges ? &paramChanges : nullptr;

    if (state.processor->process(state.pd) != kResultOk) {
        std::fprintf(stderr, "VST3: process() failed\n");
        return 0;
    }

    // --- Emit MIDI OUT (if any) back into your ring buffers ---
    // We’ll map everything to linear bus 0 by default (common case).
    // Extend here if you want true multi-bus routing.
    for (int32 i = 0, N = outEvents.getEventCount(); i < N; ++i) {
        Event e{};
        if (outEvents.getEvent(i, e) != kResultOk) {
            continue;
        }

        MidiEvent me{};
        me.frame = std::min(std::max(0, (int)e.sampleOffset), std::max(0, p_frames - 1));

        switch (e.type) {
        case Event::kNoteOnEvent:
            me.data[0] = 0x90 | (e.noteOn.channel & 0x0F);
            me.data[1] = (uint8_t)e.noteOn.pitch;
            me.data[2] = (uint8_t)std::round(e.noteOn.velocity * 127.f);
            break;
        case Event::kNoteOffEvent:
            me.data[0] = 0x80 | (e.noteOff.channel & 0x0F);
            me.data[1] = (uint8_t)e.noteOff.pitch;
            me.data[2] = (uint8_t)std::round(e.noteOff.velocity * 127.f);
            break;
        default:
            continue; // ignore other types for now
        }

        // Write to first linear MIDI out “port” if present
        if (!midi_output_buffer.empty()) {
            write_midi_out(/*bus=*/0, me);
        }
    }

    return p_frames;
}

int Vst3Host::get_input_channel_count() {
    return audio_in_ptrs.size();
}

int Vst3Host::get_output_channel_count() {
    return audio_out_ptrs.size();
}

void Vst3Host::dump_plugin_features() const {
    // Minimal: no-op
}

void Vst3Host::dump_host_features() const {
    // Minimal: no-op
}

void Vst3Host::dump_ports() const {
    // Minimal: no-op
}

int Vst3Host::get_input_midi_count() {
    return midi_input_buffer.size();
}

int Vst3Host::get_output_midi_count() {
    return midi_output_buffer.size();
}

int Vst3Host::get_input_parameter_count() {
    return parameter_input_buffer.size();
}

int Vst3Host::get_output_parameter_count() {
    return parameter_output_buffer.size();
}

float *Vst3Host::get_input_channel_buffer(int p_channel) {
    return audio_in_ptrs[p_channel];
}

float *Vst3Host::get_output_channel_buffer(int p_channel) {
    return audio_out_ptrs[p_channel];
}

void Vst3Host::write_midi_in(int p_bus, const MidiEvent &p_midi_event) {
    if (p_bus >= midi_input_buffer.size()) {
        return;
    }

    int event[MidiEvent::DATA_SIZE];

    for (int i = 0; i < MidiEvent::DATA_SIZE; i++) {
        event[i] = p_midi_event.data[i];
    }

    midi_input_buffer[p_bus].write_channel(event, MidiEvent::DATA_SIZE);
}

bool Vst3Host::read_midi_in(int p_bus, MidiEvent &p_midi_event) {
    if (p_bus >= midi_input_buffer.size()) {
        return 0;
    }

    int event[MidiEvent::DATA_SIZE];
    int read = midi_input_buffer[p_bus].read_channel(event, MidiEvent::DATA_SIZE);

    if (read == MidiEvent::DATA_SIZE) {
        for (int i = 0; i < MidiEvent::DATA_SIZE; i++) {
            p_midi_event.data[i] = event[i];
        }
        midi_input_buffer[p_bus].update_read_index(MidiEvent::DATA_SIZE);
    }

    return read > 0;
}

void Vst3Host::write_midi_out(int p_bus, const MidiEvent &p_midi_event) {
    if (p_bus >= midi_output_buffer.size()) {
        return;
    }

    int event[MidiEvent::DATA_SIZE];

    for (int i = 0; i < MidiEvent::DATA_SIZE; i++) {
        event[i] = p_midi_event.data[i];
    }

    midi_output_buffer[p_bus].write_channel(event, MidiEvent::DATA_SIZE);
}

bool Vst3Host::read_midi_out(int p_bus, MidiEvent &p_midi_event) {
    if (p_bus >= midi_output_buffer.size()) {
        return 0;
    }

    int event[MidiEvent::DATA_SIZE];
    int read = midi_output_buffer[p_bus].read_channel(event, MidiEvent::DATA_SIZE);

    if (read == MidiEvent::DATA_SIZE) {
        for (int i = 0; i < MidiEvent::DATA_SIZE; i++) {
            p_midi_event.data[i] = event[i];
        }
    }

    midi_output_buffer[p_bus].update_read_index(MidiEvent::DATA_SIZE);

    return read > 0;
}

const host::Vst3Parameter *Vst3Host::get_input_parameter(int p_index) {
    if (p_index < parameter_inputs.size()) {
        return &parameter_inputs[p_index];
    } else {
        return NULL;
    }
}

const host::Vst3Parameter *Vst3Host::get_output_parameter(int p_index) {
    if (p_index < parameter_outputs.size()) {
        return &parameter_outputs[p_index];
    } else {
        return NULL;
    }
}

float Vst3Host::get_input_parameter_value(int p_index) {
    if (p_index < parameter_inputs.size()) {
        return parameter_input_buffer[p_index].value;
    } else {
        return 0;
    }
}

float Vst3Host::get_output_parameter_value(int p_index) {
    if (p_index < parameter_outputs.size()) {
        return parameter_output_buffer[p_index].value;
    } else {
        return 0;
    }
}

void Vst3Host::set_input_parameter_value(int p_index, float p_value) {
    if (p_index < parameter_inputs.size()) {
        parameter_input_buffer[p_index].value = p_value;
        parameter_input_buffer[p_index].dirty = true;
    }
}

void Vst3Host::set_output_parameter_value(int p_index, float p_value) {
    if (p_index < parameter_outputs.size()) {
        parameter_output_buffer[p_index].value = p_value;
    }
}
