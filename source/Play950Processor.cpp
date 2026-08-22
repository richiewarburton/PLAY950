#include "Play950Processor.h"

#include "Play950Ids.h"
#include "Play950Messages.h"
#include "content/ContentLoader.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstmessage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

namespace e45recordings::play950 {
namespace {

std::vector<std::byte> readBinaryFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        throw std::runtime_error("could not open development fixture");
    const auto fileSize = stream.tellg();
    if (fileSize <= 0)
        throw std::runtime_error("development fixture is empty");
    stream.seekg(0);
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    stream.read(reinterpret_cast<char*>(bytes.data()), fileSize);
    if (!stream)
        throw std::runtime_error("could not read development fixture");
    return bytes;
}

constexpr std::size_t maximumStreamStateBytes = 64U * 1024U * 1024U;

std::vector<std::byte> readStateStream(Steinberg::IBStream* stream) {
    if (!stream)
        throw std::runtime_error("null project-state stream");
    std::vector<std::byte> bytes;
    std::array<std::byte, 64U * 1024U> buffer {};
    while (bytes.size() <= maximumStreamStateBytes) {
        Steinberg::int32 bytesRead = 0;
        const auto result = stream->read(buffer.data(), static_cast<Steinberg::int32>(buffer.size()),
                                         &bytesRead);
        if (bytesRead < 0 || bytesRead > static_cast<Steinberg::int32>(buffer.size()))
            throw std::runtime_error("invalid project-state stream read");
        bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + bytesRead);
        if (bytes.size() > maximumStreamStateBytes)
            throw std::runtime_error("project-state stream is too large");
        if (bytesRead == 0 || result != Steinberg::kResultOk)
            break;
    }
    return bytes;
}

bool writeStateStream(Steinberg::IBStream* stream, std::span<const std::byte> bytes) {
    if (!stream)
        return false;
    std::size_t position = 0;
    while (position < bytes.size()) {
        const auto count = static_cast<Steinberg::int32>(std::min<std::size_t>(
            bytes.size() - position, static_cast<std::size_t>(std::numeric_limits<Steinberg::int32>::max())));
        Steinberg::int32 bytesWritten = 0;
        if (stream->write(const_cast<std::byte*>(bytes.data() + position), count, &bytesWritten) !=
                Steinberg::kResultOk ||
            bytesWritten != count)
            return false;
        position += static_cast<std::size_t>(bytesWritten);
    }
    return true;
}

std::vector<audio::PreparedKeygroup> prepareKeygroups(
    const formats::P9Program& program,
    std::span<const formats::P9ResolvedKeygroup> resolved) {
    std::vector<audio::PreparedKeygroup> keygroups;
    keygroups.reserve(program.keygroups.size());
    for (std::size_t index = 0; index < program.keygroups.size(); ++index) {
        const auto& source = program.keygroups[index];
        keygroups.push_back({
            source.lowKey, source.highKey, resolved[index].softSampleIndex,
            resolved[index].loudSampleIndex, source.velocityThreshold,
            source.softTuning.rawSixteenths, source.loudTuning.rawSixteenths,
            source.output, source.oneShot, source.constantPitch,
            source.amplitudeEnvelope, source.vcfEnvelope, source.vcfAmount,
            source.velocityToLoudness, source.velocityToFilter,
            source.keyboardToFilter, source.softFilter, source.loudFilter,
            source.softLoudness, source.loudLoudness,
            source.midiChannelOffset});
    }
    return keygroups;
}

} // namespace

Processor::Processor() {
    setControllerClass(ControllerUID);
}

Processor::~Processor() {
    clearPrograms();
}

Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context) {
    const auto result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultOk)
        return result;

    addEventInput(STR16("MIDI In"), 16);
    addAudioOutput(STR16("All(00)"), Steinberg::Vst::SpeakerArr::kMono);
    addAudioOutput(STR16("Mono(01)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Mono(02)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Mono(03)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Mono(04)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Mono(05)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Mono(06)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Mono(07)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Mono(08)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Left(09)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    addAudioOutput(STR16("Right(10)"), Steinberg::Vst::SpeakerArr::kMono,
                   Steinberg::Vst::BusTypes::kAux);
    if (!loadDevelopmentProgram())
        sampleLoaded_ = loadDevelopmentSample();
    return Steinberg::kResultOk;
}

bool Processor::loadDevelopmentProgram() {
#if defined(PLAY950_DEVELOPMENT_P9_PATH) && defined(PLAY950_DEVELOPMENT_FIXTURE_DIR)
    try {
        return installProjectState(content::loadP9WithLinkedSamples(
            std::filesystem::path(PLAY950_DEVELOPMENT_P9_PATH)));
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

bool Processor::installProjectState(state::ProjectState projectState) {
    try {
        pitchBendRangeSemitones_.store(
            static_cast<int>(projectState.pitchBendRangeSemitones), std::memory_order_relaxed);
        midiOmni_.store(projectState.midiOmni, std::memory_order_relaxed);
        basicMidiChannel_.store(
            static_cast<int>(projectState.basicMidiChannel), std::memory_order_relaxed);
        if (!projectState.hasProgram()) {
            clearPrograms();
            return true;
        }

        auto loaded = prepareProjectState(std::move(projectState));
        clearPrograms();
        activeProgram_.store(loaded.release(), std::memory_order_release);
        sampleLoaded_ = false;
        return true;
    } catch (...) {
        return false;
    }
}

std::unique_ptr<Processor::LoadedProgram> Processor::prepareProjectState(
    state::ProjectState projectState) const {
    auto loaded = std::make_unique<LoadedProgram>();
    const auto program = formats::parseP9(projectState.p9);
    std::vector<formats::S9Sample> parsedSamples;
    parsedSamples.reserve(projectState.s9Samples.size());
    for (const auto& bytes : projectState.s9Samples)
        parsedSamples.push_back(formats::parseUncompressedS9(bytes));
    const auto resolved = formats::resolveP9Samples(program, parsedSamples);
    audio::PreparedProgram prepared;
    prepared.samples.reserve(parsedSamples.size());
    loaded->sampleMetadata.reserve(parsedSamples.size());
    for (auto& sample : parsedSamples) {
        auto sampleFrames = std::move(sample.samples12);
        loaded->sampleMetadata.push_back(sample);
        sample.samples12 = std::move(sampleFrames);
        prepared.samples.push_back(audio::prepareSample(std::move(sample)));
    }
    prepared.keygroups = prepareKeygroups(program, resolved);
    loaded->voices.setProgram(std::move(prepared));
    loaded->voices.setMidiReception(projectState.midiOmni,
                                    static_cast<int>(projectState.basicMidiChannel));
    loaded->p9 = projectState.p9;
    loaded->baseState = std::make_shared<const state::ProjectState>(
        std::move(projectState));
    return loaded;
}

bool Processor::queueProjectState(state::ProjectState projectState) {
    try {
        return queueLoadedProgram(prepareProjectState(std::move(projectState)));
    } catch (...) {
        return false;
    }
}

bool Processor::queueProgramUpdate(std::vector<std::byte> p9Data) {
    try {
        auto* current = pendingProgram_.load(std::memory_order_acquire);
        if (!current)
            current = activeProgram_.load(std::memory_order_acquire);
        if (!current || !current->baseState)
            return false;
        const auto program = formats::parseP9(p9Data);
        const auto resolved = formats::resolveP9Samples(
            program, current->sampleMetadata);
        auto loaded = std::make_unique<LoadedProgram>();
        loaded->baseState = current->baseState;
        loaded->p9 = std::move(p9Data);
        loaded->sampleMetadata = current->sampleMetadata;
        loaded->voices.setProgram(
            current->voices.preparedSamples(),
            prepareKeygroups(program, resolved));
        loaded->voices.setMidiReception(
            midiOmni_.load(std::memory_order_relaxed),
            basicMidiChannel_.load(std::memory_order_relaxed));
        return queueLoadedProgram(std::move(loaded), false);
    } catch (...) {
        return false;
    }
}

bool Processor::queueLoadedProgram(
    std::unique_ptr<LoadedProgram> loaded,
    bool adoptStoredSettings) {
    try {
        if (!loaded || !loaded->baseState)
            return false;
        if (adoptStoredSettings) {
            pitchBendRangeSemitones_.store(
                static_cast<int>(loaded->baseState->pitchBendRangeSemitones),
                std::memory_order_relaxed);
            midiOmni_.store(loaded->baseState->midiOmni, std::memory_order_relaxed);
            basicMidiChannel_.store(
                static_cast<int>(loaded->baseState->basicMidiChannel),
                std::memory_order_relaxed);
        }
        if (auto* retired = retiredProgram_.exchange(nullptr, std::memory_order_acq_rel))
            delete retired;
        // A stopped or newly created host track may not call process() between
        // two editor selections. Replace the still-pending state rather than
        // reporting a false "processor busy" failure. Atomic exchange gives
        // ownership of the displaced state exclusively to this control thread;
        // if the audio thread already adopted it, exchange returns nullptr.
        if (auto* displaced = pendingProgram_.exchange(loaded.release(),
                                                       std::memory_order_acq_rel))
            delete displaced;
        return true;
    } catch (...) {
        return false;
    }
}

Processor::LoadedProgram* Processor::adoptPendingProgram(bool& adopted) noexcept {
    adopted = false;
    if (auto* pending = pendingProgram_.exchange(nullptr, std::memory_order_acq_rel)) {
        adopted = true;
        auto* previous = activeProgram_.exchange(pending, std::memory_order_acq_rel);
        auto* expected = static_cast<LoadedProgram*>(nullptr);
        if (previous && !retiredProgram_.compare_exchange_strong(
                            expected, previous, std::memory_order_release,
                            std::memory_order_relaxed)) {
            // The control thread always reclaims the retired slot before queuing
            // another program, so reaching this branch would violate that
            // single-pending-program contract. Keep the old allocation alive
            // rather than deleting it on the audio thread.
        }
    }
    return activeProgram_.load(std::memory_order_acquire);
}

void Processor::clearPrograms() noexcept {
    std::array<LoadedProgram*, 3> programs {
        activeProgram_.exchange(nullptr, std::memory_order_acq_rel),
        pendingProgram_.exchange(nullptr, std::memory_order_acq_rel),
        retiredProgram_.exchange(nullptr, std::memory_order_acq_rel)};
    for (std::size_t index = 0; index < programs.size(); ++index) {
        if (!programs[index])
            continue;
        const bool alreadyDeleted = std::find(programs.begin(), programs.begin() + index,
                                              programs[index]) != programs.begin() + index;
        if (!alreadyDeleted)
            delete programs[index];
    }
}

bool Processor::loadDevelopmentSample() {
#if defined(PLAY950_DEVELOPMENT_S9_PATH)
    try {
        auto parsed = formats::parseUncompressedS9(readBinaryFile(PLAY950_DEVELOPMENT_S9_PATH));
        voicePool_.setSample(audio::prepareSample(std::move(parsed)));
        return true;
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

Steinberg::tresult PLUGIN_API Processor::setState(Steinberg::IBStream* stream) {
    try {
        auto restored = state::deserializeProjectState(readStateStream(stream));
        return installProjectState(std::move(restored)) ? Steinberg::kResultOk
                                                        : Steinberg::kResultFalse;
    } catch (...) {
        return Steinberg::kResultFalse;
    }
}

Steinberg::tresult PLUGIN_API Processor::getState(Steinberg::IBStream* stream) {
    try {
        auto* loaded = pendingProgram_.load(std::memory_order_acquire);
        if (!loaded)
            loaded = activeProgram_.load(std::memory_order_acquire);
        auto currentState = loaded && loaded->baseState
            ? *loaded->baseState : state::ProjectState {};
        if (loaded)
            currentState.p9 = loaded->p9;
        currentState.pitchBendRangeSemitones = static_cast<std::uint32_t>(
            pitchBendRangeSemitones_.load(std::memory_order_relaxed));
        currentState.midiOmni = midiOmni_.load(std::memory_order_relaxed);
        currentState.basicMidiChannel = static_cast<std::uint32_t>(
            basicMidiChannel_.load(std::memory_order_relaxed));
        const auto bytes = state::serializeProjectState(currentState);
        return writeStateStream(stream, bytes) ? Steinberg::kResultOk : Steinberg::kResultFalse;
    } catch (...) {
        return Steinberg::kResultFalse;
    }
}

Steinberg::tresult PLUGIN_API Processor::notify(Steinberg::Vst::IMessage* message) {
    if (!message)
        return AudioEffect::notify(message);
    const bool isProjectState = Steinberg::FIDStringsEqual(
        message->getMessageID(), loadProjectStateMessage);
    const bool isProgramUpdate = Steinberg::FIDStringsEqual(
        message->getMessageID(), updateProgramMessage);
    if (!isProjectState && !isProgramUpdate)
        return AudioEffect::notify(message);
    const void* data = nullptr;
    Steinberg::uint32 size = 0;
    if (!message->getAttributes() ||
        message->getAttributes()->getBinary(
            isProjectState ? projectStateAttribute : programDataAttribute,
            data,
            size) !=
            Steinberg::kResultOk ||
        !data)
        return Steinberg::kInvalidArgument;
    try {
        const auto bytes = std::span(static_cast<const std::byte*>(data),
                                     static_cast<std::size_t>(size));
        const bool queued = isProjectState
            ? queueProjectState(state::deserializeProjectState(bytes))
            : queueProgramUpdate(std::vector<std::byte>(bytes.begin(), bytes.end()));
        return queued ? Steinberg::kResultOk : Steinberg::kResultFalse;
    } catch (...) {
        return Steinberg::kResultFalse;
    }
}

Steinberg::tresult PLUGIN_API Processor::setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs,
    Steinberg::int32 numInputs,
    Steinberg::Vst::SpeakerArrangement* outputs,
    Steinberg::int32 numOutputs) {
    if (numInputs != 0 || numOutputs != 11)
        return Steinberg::kResultFalse;

    for (Steinberg::int32 bus = 0; bus < numOutputs; ++bus) {
        if (outputs[bus] != Steinberg::Vst::SpeakerArr::kMono)
            return Steinberg::kResultFalse;
    }
    return AudioEffect::setBusArrangements(inputs, numInputs, outputs, numOutputs);
}

Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data) {
    if (data.symbolicSampleSize != Steinberg::Vst::kSample32)
        return Steinberg::kResultFalse;

    for (Steinberg::int32 bus = 0; bus < data.numOutputs; ++bus) {
        for (Steinberg::int32 channel = 0; channel < data.outputs[bus].numChannels; ++channel) {
            std::fill_n(data.outputs[bus].channelBuffers32[channel], data.numSamples, 0.0F);
        }
        data.outputs[bus].silenceFlags = 0;
    }

    if (data.numOutputs < 1 || data.outputs[0].numChannels < 1)
        return Steinberg::kResultOk;

    auto* all = data.outputs[0].channelBuffers32[0];
    bool adoptedProgram = false;
    auto* loadedProgram = adoptPendingProgram(adoptedProgram);
    std::array<bool, audio::ProgramVoicePool::midiChannelCount> bendChanged {};
    bool bendRangeChanged = false;
    bool midiReceptionChanged = false;
    if (data.inputParameterChanges) {
        for (Steinberg::int32 index = 0;
             index < data.inputParameterChanges->getParameterCount(); ++index) {
            auto* queue = data.inputParameterChanges->getParameterData(index);
            if (!queue || queue->getPointCount() == 0)
                continue;
            Steinberg::int32 offset = 0;
            Steinberg::Vst::ParamValue value = 0.0;
            if (queue->getPoint(queue->getPointCount() - 1, offset, value) != Steinberg::kResultOk)
                continue;
            const auto parameterId = queue->getParameterId();
            const auto bendChannel = channelForPitchBendParameter(parameterId);
            if (bendChannel >= 0) {
                pitchBends_[static_cast<std::size_t>(bendChannel)] =
                    static_cast<float>(std::clamp(value, 0.0, 1.0) * 2.0 - 1.0);
                bendChanged[static_cast<std::size_t>(bendChannel)] = true;
            } else if (parameterId == pitchBendRangeParameter) {
                pitchBendRangeSemitones_.store(
                    std::clamp(static_cast<int>(std::lround(1.0 + value * 11.0)), 1, 12),
                    std::memory_order_relaxed);
                bendRangeChanged = true;
            } else if (parameterId == midiReceiveModeParameter) {
                midiOmni_.store(value < 0.5, std::memory_order_relaxed);
                midiReceptionChanged = true;
            } else if (parameterId == basicMidiChannelParameter) {
                basicMidiChannel_.store(
                    std::clamp(static_cast<int>(std::lround(1.0 + value * 15.0)), 1, 16),
                    std::memory_order_relaxed);
                midiReceptionChanged = true;
            }
        }
    }
    const auto bendRange = pitchBendRangeSemitones_.load(std::memory_order_relaxed);
    if (bendRangeChanged || bendChanged[0])
        voicePool_.setPitchBend(pitchBends_[0], bendRange);
    if (loadedProgram) {
        if (adoptedProgram || bendRangeChanged) {
            for (std::size_t channel = 0; channel < pitchBends_.size(); ++channel)
                loadedProgram->voices.setPitchBend(
                    static_cast<std::int32_t>(channel), pitchBends_[channel], bendRange);
        } else {
            for (std::size_t channel = 0; channel < bendChanged.size(); ++channel) {
                if (bendChanged[channel])
                    loadedProgram->voices.setPitchBend(
                        static_cast<std::int32_t>(channel), pitchBends_[channel], bendRange);
            }
        }
        if (midiReceptionChanged)
            loadedProgram->voices.setMidiReception(
                midiOmni_.load(std::memory_order_relaxed),
                basicMidiChannel_.load(std::memory_order_relaxed));
    }
    voicePool_.setHostSampleRate(processSetup.sampleRate);
    if (loadedProgram)
        loadedProgram->voices.setHostSampleRate(processSetup.sampleRate);
    Steinberg::int32 renderedUntil = 0;
    const auto renderSegment = [&](Steinberg::int32 start, Steinberg::int32 count) {
        if (loadedProgram) {
            audio::ProgramVoicePool::OutputBuffers outputs {};
            for (Steinberg::int32 bus = 0;
                 bus < data.numOutputs && bus < static_cast<Steinberg::int32>(outputs.size()); ++bus) {
                if (data.outputs[bus].numChannels > 0)
                    outputs[static_cast<std::size_t>(bus)] =
                        data.outputs[bus].channelBuffers32[0] + start;
            }
            loadedProgram->voices.renderAdd(outputs, count);
        } else {
            voicePool_.renderAdd(all + start, count);
        }
    };
    if (data.inputEvents) {
        Steinberg::Vst::Event event {};
        for (Steinberg::int32 i = 0; i < data.inputEvents->getEventCount(); ++i) {
            if (data.inputEvents->getEvent(i, event) != Steinberg::kResultOk)
                continue;
            const auto offset = std::clamp(event.sampleOffset, renderedUntil, data.numSamples);
            renderSegment(renderedUntil, offset - renderedUntil);
            renderedUntil = offset;
            if (event.type == Steinberg::Vst::Event::kNoteOnEvent && event.noteOn.velocity > 0.0F) {
                if (loadedProgram)
                    loadedProgram->voices.noteOn(event.noteOn.channel, event.noteOn.pitch,
                                                 event.noteOn.noteId, event.noteOn.velocity);
                else
                    voicePool_.noteOn(event.noteOn.pitch, event.noteOn.noteId,
                                      event.noteOn.velocity);
            } else if (event.type == Steinberg::Vst::Event::kNoteOffEvent) {
                if (loadedProgram)
                    loadedProgram->voices.noteOff(event.noteOff.channel, event.noteOff.pitch,
                                                  event.noteOff.noteId);
                else
                    voicePool_.noteOff(event.noteOff.pitch, event.noteOff.noteId);
            } else if (event.type == Steinberg::Vst::Event::kNoteOnEvent &&
                       event.noteOn.velocity <= 0.0F) {
                if (loadedProgram)
                    loadedProgram->voices.noteOff(event.noteOn.channel, event.noteOn.pitch,
                                                  event.noteOn.noteId);
                else
                    voicePool_.noteOff(event.noteOn.pitch, event.noteOn.noteId);
            }
        }
    }
    renderSegment(renderedUntil, data.numSamples - renderedUntil);
    return Steinberg::kResultOk;
}

} // namespace e45recordings::play950
