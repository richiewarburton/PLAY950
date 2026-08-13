#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

#include "audio/ProgramVoicePool.h"
#include "formats/S9.h"
#include "audio/SampleVoicePool.h"
#include "state/ProjectState.h"

#include <cstdint>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace e45recordings::play950 {

class Processor final : public Steinberg::Vst::AudioEffect {
public:
    Processor();
    ~Processor() override;
    static Steinberg::FUnknown* createInstance(void*) { return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor); }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* inputs,
        Steinberg::int32 numInputs,
        Steinberg::Vst::SpeakerArrangement* outputs,
        Steinberg::int32 numOutputs) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* stream) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* stream) override;
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override;

private:
    struct LoadedProgram {
        audio::ProgramVoicePool voices;
        state::ProjectState state;
    };
    static_assert(std::atomic<LoadedProgram*>::is_always_lock_free);

    bool loadDevelopmentProgram();
    bool loadDevelopmentSample();
    bool installProjectState(state::ProjectState projectState);
    [[nodiscard]] std::unique_ptr<LoadedProgram> prepareProjectState(
        state::ProjectState projectState) const;
    bool queueProjectState(state::ProjectState projectState);
    [[nodiscard]] LoadedProgram* adoptPendingProgram(bool& adopted) noexcept;
    void clearPrograms() noexcept;

    audio::SampleVoicePool voicePool_;
    bool sampleLoaded_ {false};
    std::atomic<LoadedProgram*> activeProgram_ {nullptr};
    std::atomic<LoadedProgram*> pendingProgram_ {nullptr};
    std::atomic<LoadedProgram*> retiredProgram_ {nullptr};
    std::atomic<int> pitchBendRangeSemitones_ {2};
    std::atomic<bool> midiOmni_ {true};
    std::atomic<int> basicMidiChannel_ {1};
    std::array<float, audio::ProgramVoicePool::midiChannelCount> pitchBends_ {};
};

} // namespace e45recordings::play950
