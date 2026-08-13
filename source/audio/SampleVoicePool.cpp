#include "audio/SampleVoicePool.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace e45recordings::play950::audio {

PreparedSample prepareSample(formats::S9Sample parsed) {
    PreparedSample prepared;
    prepared.samples12 = std::move(parsed.samples12);
    prepared.sampleRate = parsed.sampleRate;
    prepared.nominalPitchSixteenths = parsed.nominalPitchSixteenths;
    prepared.playbackStart = parsed.playbackStart;
    prepared.playbackEnd = parsed.playbackEnd;
    prepared.loopStart = parsed.loopStart();
    prepared.playbackMode = parsed.playbackMode;
    prepared.direction = parsed.direction;
    prepared.loudnessOffset = parsed.loudnessOffset;
    return prepared;
}

void SampleVoicePool::setSample(PreparedSample sample) {
    sample_ = std::move(sample);
    for (auto& voice : voices_)
        voice = {};
    nextAge_ = 1;
}

void SampleVoicePool::setHostSampleRate(double sampleRate) noexcept {
    if (sampleRate > 0.0)
        hostSampleRate_ = sampleRate;
}

SampleVoicePool::Voice& SampleVoicePool::voiceForNewNote() noexcept {
    if (auto inactive = std::find_if(voices_.begin(), voices_.end(),
                                     [](const Voice& voice) { return !voice.active; });
        inactive != voices_.end())
        return *inactive;
    return *std::min_element(voices_.begin(), voices_.end(),
                             [](const Voice& a, const Voice& b) { return a.age < b.age; });
}

void SampleVoicePool::noteOn(std::int32_t pitch, std::int32_t noteId, float velocity) noexcept {
    if (sample_.samples12.empty() || sample_.sampleRate == 0 ||
        sample_.playbackEnd <= sample_.playbackStart || velocity <= 0.0F)
        return;
    auto& voice = voiceForNewNote();
    voice.active = true;
    voice.pitch = pitch;
    voice.noteId = noteId;
    voice.velocity = velocity;
    voice.playbackDirection = sample_.direction == formats::S9Direction::reverse ? -1 : 1;
    voice.position = voice.playbackDirection > 0
                         ? static_cast<double>(sample_.playbackStart)
                         : static_cast<double>(sample_.playbackEnd - 1);
    voice.age = nextAge_++;
    const double semitones =
        (static_cast<double>(pitch * 16) - sample_.nominalPitchSixteenths) / 192.0;
    voice.increment = static_cast<double>(sample_.sampleRate) / hostSampleRate_ *
                      std::pow(2.0, semitones);
}

void SampleVoicePool::noteOff(std::int32_t pitch, std::int32_t noteId) noexcept {
    for (auto& voice : voices_) {
        const bool idMatches = noteId >= 0 ? voice.noteId == noteId : voice.pitch == pitch;
        if (voice.active && idMatches) {
            voice.active = false;
            if (noteId >= 0)
                return;
        }
    }
}

void SampleVoicePool::setPitchBend(float normalizedBipolar, int rangeSemitones) noexcept {
    const double semitones = static_cast<double>(std::clamp(normalizedBipolar, -1.0F, 1.0F)) *
                             std::clamp(rangeSemitones, 1, 12);
    pitchBendRatio_ = std::pow(2.0, semitones / 12.0);
}

float SampleVoicePool::nextSample(Voice& voice) noexcept {
    const double start = static_cast<double>(sample_.playbackStart);
    const double end = static_cast<double>(sample_.playbackEnd);
    const double high = end - 1.0;
    const double loopStart = static_cast<double>(sample_.loopStart);

    if (sample_.playbackMode == formats::S9PlaybackMode::alternatingLoop &&
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
        if (sample_.playbackMode == formats::S9PlaybackMode::oneShot || loopStart >= end) {
            voice.active = false;
            return 0.0F;
        }
        const double span = end - loopStart;
        voice.position = loopStart + std::fmod(voice.position - end, span);
    } else if (voice.playbackDirection < 0 &&
               voice.position < (sample_.playbackMode == formats::S9PlaybackMode::oneShot
                                     ? start : loopStart)) {
        if (sample_.playbackMode == formats::S9PlaybackMode::oneShot || loopStart >= end) {
            voice.active = false;
            return 0.0F;
        }
        const double span = end - loopStart;
        const double remainder = std::fmod(loopStart - voice.position, span);
        voice.position = remainder == 0.0 ? loopStart : end - remainder;
    }
    const auto index = static_cast<std::size_t>(voice.position);
    const auto next = std::min(index + 1, static_cast<std::size_t>(sample_.playbackEnd - 1));
    const float fraction = static_cast<float>(voice.position - static_cast<double>(index));
    const float first = static_cast<float>(sample_.samples12[index]);
    const float second = static_cast<float>(sample_.samples12[next]);
    voice.position += voice.increment * pitchBendRatio_ *
                      static_cast<double>(voice.playbackDirection);
    return ((first + (second - first) * fraction) / 2048.0F) * voice.velocity;
}

void SampleVoicePool::renderAdd(float* output, std::int32_t frameCount) noexcept {
    if (!output || frameCount <= 0)
        return;
    for (std::int32_t frame = 0; frame < frameCount; ++frame) {
        float mixed = 0.0F;
        for (auto& voice : voices_) {
            if (voice.active)
                mixed += nextSample(voice);
        }
        output[frame] += mixed;
    }
}

std::size_t SampleVoicePool::activeVoiceCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        voices_.begin(), voices_.end(), [](const Voice& voice) { return voice.active; }));
}

bool SampleVoicePool::isNoteActive(std::int32_t pitch, std::int32_t noteId) const noexcept {
    return std::any_of(voices_.begin(), voices_.end(), [&](const Voice& voice) {
        return voice.active && voice.pitch == pitch && (noteId < 0 || voice.noteId == noteId);
    });
}

} // namespace e45recordings::play950::audio
