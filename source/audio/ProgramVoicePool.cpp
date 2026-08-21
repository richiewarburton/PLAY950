#include "audio/ProgramVoicePool.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace e45recordings::play950::audio {
namespace {

int sideFor(formats::P9Output output) noexcept {
    if (output.kind() == formats::P9OutputKind::left)
        return 0;
    if (output.kind() == formats::P9OutputKind::right)
        return 1;
    if (output.kind() == formats::P9OutputKind::mono)
        return output.raw < 4 ? 0 : 1;
    return -1;
}

double envelopeTimeSeconds(int stage, std::uint8_t value) noexcept {
    if (value == 0)
        return 0.0;
    const double normalized = static_cast<double>(value) / 99.0;
    switch (stage) {
        case 0:
            return 0.002 + 12.0 * std::pow(normalized, 3.5);
        case 1:
            return 0.002 + 20.0 * std::pow(normalized, 4.5);
        case 3:
            return 0.002 + 22.0 * std::pow(normalized, 5.2);
        case 2:
        case 4:
            return 0.0;
    }
    return 0.0;
}

float cutoffForControl(float control) noexcept {
    constexpr std::array<float, 5> controls {0.0F, 20.0F, 40.0F, 60.0F, 80.0F};
    // Perceptual anchors fitted to the supplied S950 noise/saw recordings.
    constexpr std::array<float, 5> frequencies {180.0F, 220.0F, 700.0F, 5'500.0F,
                                                24'000.0F};
    control = std::clamp(control, 0.0F, 80.0F);
    for (std::size_t index = 1; index < controls.size(); ++index) {
        if (control > controls[index])
            continue;
        const float position = (control - controls[index - 1]) /
                               (controls[index] - controls[index - 1]);
        return std::exp(std::log(frequencies[index - 1]) * (1.0F - position) +
                        std::log(frequencies[index]) * position);
    }
    return frequencies.back();
}

float loudnessGain(int sampleOffset, int layerOffset, std::uint8_t sensitivity,
                   float velocity) noexcept {
    // S900/S950 loudness values are arbitrary -50...+50 control units, not dB.
    // Treat the S9 and P9 values as cumulative offsets around unity, with the
    // native +50 ceiling at 2x. Velocity sensitivity subtracts from the
    // remaining control range, so 00 is velocity-independent and +50 removes
    // velocity dynamics as described by the S950 manual.
    const float offset = std::clamp(static_cast<float>(sampleOffset + layerOffset),
                                    -50.0F, 50.0F);
    const float maximum = 1.0F + offset / 50.0F;
    const float velocityDepth =
        (static_cast<float>(std::min<std::uint8_t>(sensitivity, 99)) / 99.0F) *
        (2.0F - maximum);
    return std::clamp(maximum - velocityDepth *
                                    (1.0F - std::clamp(velocity, 0.0F, 1.0F)),
                      0.0F, 2.0F);
}

} // namespace

void ProgramVoicePool::setProgram(PreparedProgram program) {
    program_ = std::move(program);
    for (auto& voice : voices_)
        voice = {};
    retiringMonophonicVoices_ = {};
    nextAge_ = 1;
}

void ProgramVoicePool::setHostSampleRate(double sampleRate) noexcept {
    if (sampleRate > 0.0)
        hostSampleRate_ = sampleRate;
}

void ProgramVoicePool::setMidiReception(bool omni, int basicMidiChannel) noexcept {
    midiOmni_ = omni;
    basicMidiChannel_ = std::clamp(basicMidiChannel, 1, 16);
}

ProgramVoicePool::Voice& ProgramVoicePool::voiceForNewNote() noexcept {
    if (auto inactive = std::find_if(voices_.begin(), voices_.end(),
                                     [](const Voice& voice) { return !voice.active; });
        inactive != voices_.end())
        return *inactive;
    return *std::min_element(voices_.begin(), voices_.end(),
                             [](const Voice& a, const Voice& b) { return a.age < b.age; });
}

void ProgramVoicePool::enforceOutputLimits(formats::P9Output output) noexcept {
    const int side = sideFor(output);
    if (side < 0)
        return;
    std::size_t count = 0;
    Voice* oldest = nullptr;
    for (auto& voice : voices_) {
        if (!voice.active || sideFor(voice.output) != side)
            continue;
        ++count;
        if (!oldest || voice.age < oldest->age)
            oldest = &voice;
    }
    if (count >= 4 && oldest)
        oldest->active = false;
}

void ProgramVoicePool::initializeVoice(Voice& voice, const PendingNote& note) noexcept {
    const auto& sample = program_.samples[note.sampleIndex];
    const auto& keygroup = program_.keygroups[note.keygroupIndex];
    voice = {};
    voice.active = true;
    voice.midiChannel = note.midiChannel;
    voice.pitch = note.pitch;
    voice.noteId = note.noteId;
    voice.gain = 1.0F;
    voice.playbackDirection = sample.direction == formats::S9Direction::reverse ? -1 : 1;
    voice.position = voice.playbackDirection > 0
                         ? static_cast<double>(sample.playbackStart)
                         : static_cast<double>(sample.playbackEnd - 1);
    voice.sampleIndex = note.sampleIndex;
    voice.keygroupIndex = note.keygroupIndex;
    voice.output = keygroup.output;
    voice.oneShot = keygroup.oneShot;
    voice.velocity = note.velocity;
    voice.baseFilter = static_cast<float>(note.useLoudLayer ? keygroup.loudFilter
                                                            : keygroup.softFilter);
    const auto loudness = note.useLoudLayer ? keygroup.loudLoudness : keygroup.softLoudness;
    voice.loudnessGain = loudnessGain(sample.loudnessOffset, loudness,
                                      keygroup.velocityToLoudness, voice.velocity);
    voice.amplitude = {keygroup.amplitudeEnvelope,
                       EnvelopeState::Stage::attack, 0.0F, 0.0F, 0};
    voice.filter = {keygroup.filterEnvelope,
                    EnvelopeState::Stage::attack, 0.0F, 0.0F, 0};
    voice.age = nextAge_++;
    const double pitchSixteenths = keygroup.constantPitch
        ? static_cast<double>(sample.nominalPitchSixteenths) + note.tuningSixteenths
        : static_cast<double>(note.pitch * 16) + note.tuningSixteenths;
    const double semitones =
        (pitchSixteenths - sample.nominalPitchSixteenths) / 192.0;
    voice.increment = static_cast<double>(sample.sampleRate) / hostSampleRate_ *
                      std::pow(2.0, semitones);
}

void ProgramVoicePool::retireMonophonicVoice(const Voice& voice) noexcept {
    auto slot = std::find_if(retiringMonophonicVoices_.begin(),
                             retiringMonophonicVoices_.end(),
                             [](const RetiringVoice& retiring) {
                                 return retiring.samplesRemaining == 0;
                             });
    if (slot == retiringMonophonicVoices_.end()) {
        // Eight retriggers inside 2.5 ms already exceed the audible use case;
        // if that happens, replace the tail nearest silence.
        slot = std::min_element(retiringMonophonicVoices_.begin(),
                                retiringMonophonicVoices_.end(),
                                [](const RetiringVoice& a, const RetiringVoice& b) {
                                    return a.samplesRemaining < b.samplesRemaining;
                                });
    }
    auto& retiring = *slot;
    retiring.voice = voice;
    retiring.totalSamples = std::max<std::uint64_t>(
        1, static_cast<std::uint64_t>(
               std::llround(hostSampleRate_ * monophonicRetirementSeconds)));
    retiring.samplesRemaining = retiring.totalSamples;
}

void ProgramVoicePool::noteOn(
    std::int32_t channel, std::int32_t pitch, std::int32_t noteId, float velocity) noexcept {
    if (channel < 0 || channel >= static_cast<std::int32_t>(midiChannelCount) ||
        velocity <= 0.0F)
        return;
    const auto keygroup = std::find_if(program_.keygroups.begin(), program_.keygroups.end(),
                                       [&](const PreparedKeygroup& candidate) {
                                           const auto keygroupChannel =
                                               (basicMidiChannel_ - 1 +
                                                static_cast<int>(candidate.midiChannelOffset)) %
                                               static_cast<int>(midiChannelCount);
                                           return pitch >= candidate.lowKey &&
                                                  pitch <= candidate.highKey &&
                                                  (midiOmni_ || channel == keygroupChannel);
                                       });
    if (keygroup == program_.keygroups.end())
        return;
    const auto midiVelocity = static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(std::lround(velocity * 127.0F)), 1, 127));
    const bool useLoudLayer = midiVelocity >= keygroup->velocityThreshold;
    const auto sampleIndex = useLoudLayer ? keygroup->loudSampleIndex
                                          : keygroup->softSampleIndex;
    const auto tuning = useLoudLayer ? keygroup->loudTuningSixteenths
                                     : keygroup->softTuningSixteenths;
    if (!sampleIndex || *sampleIndex >= program_.samples.size())
        return;
    const auto& sample = program_.samples[*sampleIndex];
    if (sample.samples12.empty() || sample.sampleRate == 0 ||
        sample.playbackEnd <= sample.playbackStart)
        return;

    const PendingNote note {
        channel, pitch, noteId, static_cast<float>(midiVelocity) / 127.0F,
        *sampleIndex, static_cast<std::size_t>(keygroup - program_.keygroups.begin()),
        tuning, useLoudLayer};
    if (keygroup->output.kind() == formats::P9OutputKind::mono &&
        keygroup->output.raw < retiringMonophonicVoices_.size()) {
        auto occupied = std::find_if(voices_.begin(), voices_.end(),
                                     [&](const Voice& voice) {
            return voice.active && voice.output.kind() == formats::P9OutputKind::mono &&
                   voice.output.raw == keygroup->output.raw;
        });
        if (occupied != voices_.end()) {
            // Start the replacement exactly on the note event.  The displaced
            // post-filter voice is kept in fixed storage and ramps to silence,
            // avoiding both a hard waveform discontinuity and audio-thread allocation.
            retireMonophonicVoice(*occupied);
            initializeVoice(*occupied, note);
            return;
        }
    }

    enforceOutputLimits(keygroup->output);
    auto& voice = voiceForNewNote();
    initializeVoice(voice, note);
}

void ProgramVoicePool::noteOn(
    std::int32_t pitch, std::int32_t noteId, float velocity) noexcept {
    noteOn(0, pitch, noteId, velocity);
}

void ProgramVoicePool::noteOff(
    std::int32_t channel, std::int32_t pitch, std::int32_t noteId) noexcept {
    if (channel < 0 || channel >= static_cast<std::int32_t>(midiChannelCount))
        return;
    for (auto& voice : voices_) {
        const bool idMatches = noteId >= 0 ? voice.noteId == noteId : voice.pitch == pitch;
        if (voice.active && voice.midiChannel == channel && idMatches && !voice.oneShot) {
            if (voice.amplitude.parameters.release == 0) {
                voice.active = false;
                if (noteId >= 0)
                    return;
                continue;
            }
            voice.amplitude.releaseStart = voice.amplitude.level;
            voice.amplitude.stage = EnvelopeState::Stage::release;
            voice.amplitude.stageSamples = 0;
            voice.filter.releaseStart = voice.filter.level;
            voice.filter.stage = EnvelopeState::Stage::release;
            voice.filter.stageSamples = 0;
            if (noteId >= 0)
                return;
        }
    }
}

void ProgramVoicePool::noteOff(std::int32_t pitch, std::int32_t noteId) noexcept {
    noteOff(0, pitch, noteId);
}

void ProgramVoicePool::setPitchBend(
    std::int32_t channel, float normalizedBipolar, int rangeSemitones) noexcept {
    if (channel < 0 || channel >= static_cast<std::int32_t>(midiChannelCount))
        return;
    const double semitones = static_cast<double>(std::clamp(normalizedBipolar, -1.0F, 1.0F)) *
                             std::clamp(rangeSemitones, 1, 12);
    pitchBendRatios_[static_cast<std::size_t>(channel)] = std::pow(2.0, semitones / 12.0);
}

void ProgramVoicePool::setPitchBend(float normalizedBipolar, int rangeSemitones) noexcept {
    for (std::int32_t channel = 0;
         channel < static_cast<std::int32_t>(midiChannelCount); ++channel)
        setPitchBend(channel, normalizedBipolar, rangeSemitones);
}

float ProgramVoicePool::advanceEnvelope(EnvelopeState& envelope) const noexcept {
    for (int transition = 0; transition < 3; ++transition) {
        const auto stage = envelope.stage;
        if (stage == EnvelopeState::Stage::sustain) {
            envelope.level = static_cast<float>(envelope.parameters.sustain) / 99.0F;
            return envelope.level;
        }
        if (stage == EnvelopeState::Stage::done) {
            envelope.level = 0.0F;
            return 0.0F;
        }
        const std::uint8_t parameter = stage == EnvelopeState::Stage::attack
            ? envelope.parameters.attack
            : stage == EnvelopeState::Stage::decay ? envelope.parameters.decay
                                                   : envelope.parameters.release;
        const double seconds = envelopeTimeSeconds(static_cast<int>(stage), parameter);
        const auto duration = static_cast<std::uint64_t>(std::llround(seconds * hostSampleRate_));
        if (duration == 0) {
            if (stage == EnvelopeState::Stage::attack) {
                envelope.level = 1.0F;
                envelope.stage = EnvelopeState::Stage::decay;
            } else if (stage == EnvelopeState::Stage::decay) {
                envelope.level = static_cast<float>(envelope.parameters.sustain) / 99.0F;
                envelope.stage = EnvelopeState::Stage::sustain;
            } else {
                envelope.level = 0.0F;
                envelope.stage = EnvelopeState::Stage::done;
            }
            envelope.stageSamples = 0;
            continue;
        }
        const float position = std::min(1.0F, static_cast<float>(envelope.stageSamples + 1) /
                                                  static_cast<float>(duration));
        if (stage == EnvelopeState::Stage::attack) {
            envelope.level = std::pow(position, 0.72F);
        } else if (stage == EnvelopeState::Stage::decay) {
            const float sustain = static_cast<float>(envelope.parameters.sustain) / 99.0F;
            envelope.level = sustain + (1.0F - sustain) * std::pow(1.0F - position, 2.0F);
        } else {
            envelope.level = envelope.releaseStart * std::pow(1.0F - position, 2.0F);
        }
        ++envelope.stageSamples;
        if (envelope.stageSamples >= duration) {
            envelope.stageSamples = 0;
            envelope.stage = stage == EnvelopeState::Stage::attack
                ? EnvelopeState::Stage::decay
                : stage == EnvelopeState::Stage::decay ? EnvelopeState::Stage::sustain
                                                       : EnvelopeState::Stage::done;
        }
        return envelope.level;
    }
    return envelope.level;
}

float ProgramVoicePool::nextSample(Voice& voice) noexcept {
    const auto& sample = program_.samples[voice.sampleIndex];
    const double start = static_cast<double>(sample.playbackStart);
    const double end = static_cast<double>(sample.playbackEnd);
    const double high = end - 1.0;
    const double loopStart = static_cast<double>(sample.loopStart);

    if (sample.playbackMode == formats::S9PlaybackMode::alternatingLoop &&
        loopStart < high) {
        const double span = high - loopStart;
        if (voice.playbackDirection > 0 && voice.position > high) {
            const double phase = std::fmod(voice.position - high, span * 2.0);
            if (phase <= span) {
                voice.position = high - phase;
                voice.playbackDirection = -1;
            } else {
                voice.position = loopStart + (phase - span);
            }
        } else if (voice.playbackDirection < 0 && voice.position < loopStart) {
            const double phase = std::fmod(loopStart - voice.position, span * 2.0);
            if (phase <= span) {
                voice.position = loopStart + phase;
                voice.playbackDirection = 1;
            } else {
                voice.position = high - (phase - span);
            }
        }
    } else if (voice.playbackDirection > 0 && voice.position >= end) {
        if (sample.playbackMode == formats::S9PlaybackMode::oneShot || loopStart >= end) {
            voice.active = false;
            return 0.0F;
        }
        const double span = end - loopStart;
        voice.position = loopStart + std::fmod(voice.position - end, span);
    } else if (voice.playbackDirection < 0 &&
               voice.position < (sample.playbackMode == formats::S9PlaybackMode::oneShot
                                     ? start : loopStart)) {
        if (sample.playbackMode == formats::S9PlaybackMode::oneShot || loopStart >= end) {
            voice.active = false;
            return 0.0F;
        }
        const double span = end - loopStart;
        const double remainder = std::fmod(loopStart - voice.position, span);
        voice.position = remainder == 0.0 ? loopStart : end - remainder;
    }
    const auto index = static_cast<std::size_t>(voice.position);
    const auto next = std::min(index + 1, static_cast<std::size_t>(sample.playbackEnd - 1));
    const float fraction = static_cast<float>(voice.position - static_cast<double>(index));
    const float first = static_cast<float>(sample.samples12[index]);
    const float second = static_cast<float>(sample.samples12[next]);
    voice.position += voice.increment *
                      pitchBendRatios_[static_cast<std::size_t>(voice.midiChannel)] *
                      static_cast<double>(voice.playbackDirection);
    float value = ((first + (second - first) * fraction) / 2048.0F) * voice.gain;
    const auto& keygroup = program_.keygroups[voice.keygroupIndex];
    const float ampEnvelope = advanceEnvelope(voice.amplitude);
    const float filterEnvelope = advanceEnvelope(voice.filter);
    if (voice.amplitude.stage == EnvelopeState::Stage::done) {
        voice.active = false;
        return 0.0F;
    }

    float filterControl = voice.baseFilter;
    filterControl += static_cast<float>(keygroup.filterEnvelopeAmount) * filterEnvelope;
    filterControl += static_cast<float>(keygroup.velocityToFilter) * (voice.velocity - 0.5F);
    filterControl += static_cast<float>(keygroup.keyboardToFilter) *
                     (static_cast<float>(voice.pitch) - 60.0F) / 12.0F;
    filterControl = std::clamp(filterControl, 0.0F, 99.0F);
    if (filterControl < 80.0F) {
        const float cutoff = std::min(cutoffForControl(filterControl),
                                      static_cast<float>(hostSampleRate_ * 0.45));
        const float coefficient = 1.0F -
            std::exp(-2.0F * static_cast<float>(std::numbers::pi) * cutoff /
                     static_cast<float>(hostSampleRate_));
        for (auto& state : voice.filterState) {
            state += coefficient * (value - state);
            value = state;
        }
    }
    const float amplitudeGain = std::pow(std::clamp(ampEnvelope, 0.0F, 1.0F), 2.5F);
    return value * amplitudeGain * voice.loudnessGain;
}

void ProgramVoicePool::renderAdd(
    const OutputBuffers& outputs, std::int32_t frameCount) noexcept {
    if (frameCount <= 0)
        return;
    for (std::int32_t frame = 0; frame < frameCount; ++frame) {
        for (auto& retiring : retiringMonophonicVoices_) {
            if (retiring.samplesRemaining == 0 || !retiring.voice.active)
                continue;
            const float value = nextSample(retiring.voice);
            const float gain = static_cast<float>(retiring.samplesRemaining) /
                               static_cast<float>(retiring.totalSamples);
            addToOutput(outputs, frame, retiring.voice, value * gain);
            if (--retiring.samplesRemaining == 0)
                retiring.voice.active = false;
        }
        for (auto& voice : voices_) {
            if (!voice.active)
                continue;
            const float value = nextSample(voice);
            addToOutput(outputs, frame, voice, value);
        }
    }
}

void ProgramVoicePool::addToOutput(const OutputBuffers& outputs, std::int32_t frame,
                                   const Voice& voice, float value) const noexcept {
    if (outputs[0])
        outputs[0][frame] += value;
    switch (voice.output.kind()) {
        case formats::P9OutputKind::mono: {
            const std::size_t monoBus = static_cast<std::size_t>(voice.output.raw) + 1;
            const std::size_t sideBus = voice.output.raw < 4 ? 9 : 10;
            if (outputs[monoBus]) outputs[monoBus][frame] += value;
            if (outputs[sideBus]) outputs[sideBus][frame] += value;
            break;
        }
        case formats::P9OutputKind::left:
            if (outputs[9]) outputs[9][frame] += value;
            break;
        case formats::P9OutputKind::right:
            if (outputs[10]) outputs[10][frame] += value;
            break;
        case formats::P9OutputKind::all:
        case formats::P9OutputKind::unknown:
            break;
    }
}

std::size_t ProgramVoicePool::activeVoiceCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        voices_.begin(), voices_.end(), [](const Voice& voice) { return voice.active; }));
}

bool ProgramVoicePool::isNoteActive(
    std::int32_t channel, std::int32_t pitch, std::int32_t noteId) const noexcept {
    return std::any_of(voices_.begin(), voices_.end(), [&](const Voice& voice) {
        return voice.active && voice.midiChannel == channel && voice.pitch == pitch &&
               (noteId < 0 || voice.noteId == noteId);
    });
}

bool ProgramVoicePool::isNoteActive(std::int32_t pitch, std::int32_t noteId) const noexcept {
    return std::any_of(voices_.begin(), voices_.end(), [&](const Voice& voice) {
        return voice.active && voice.pitch == pitch && (noteId < 0 || voice.noteId == noteId);
    });
}

} // namespace e45recordings::play950::audio
