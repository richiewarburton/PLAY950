#pragma once

#include "formats/S9.h"

#include <array>
#include <cstdint>
#include <vector>

namespace e45recordings::play950::audio {

struct PreparedSample {
    std::vector<std::int16_t> samples12;
    std::uint32_t sampleRate {0};
    std::uint16_t nominalPitchSixteenths {0};
    std::uint32_t playbackStart {0};
    std::uint32_t playbackEnd {0};
    std::uint32_t loopStart {0};
    formats::S9PlaybackMode playbackMode {formats::S9PlaybackMode::oneShot};
    formats::S9Direction direction {formats::S9Direction::forward};
    std::int16_t loudnessOffset {0};
};

[[nodiscard]] PreparedSample prepareSample(formats::S9Sample parsed);

class SampleVoicePool {
public:
    static constexpr std::size_t voiceCount = 8;

    void setSample(PreparedSample sample);
    void setHostSampleRate(double sampleRate) noexcept;
    void noteOn(std::int32_t pitch, std::int32_t noteId, float velocity) noexcept;
    void noteOff(std::int32_t pitch, std::int32_t noteId) noexcept;
    void setPitchBend(float normalizedBipolar, int rangeSemitones) noexcept;
    void renderAdd(float* output, std::int32_t frameCount) noexcept;

    [[nodiscard]] std::size_t activeVoiceCount() const noexcept;
    [[nodiscard]] bool isNoteActive(std::int32_t pitch, std::int32_t noteId) const noexcept;

private:
    struct Voice {
        double position {0.0};
        double increment {1.0};
        float velocity {0.0F};
        std::uint64_t age {0};
        std::int32_t pitch {-1};
        std::int32_t noteId {-1};
        int playbackDirection {1};
        bool active {false};
    };

    [[nodiscard]] Voice& voiceForNewNote() noexcept;
    float nextSample(Voice& voice) noexcept;

    PreparedSample sample_;
    std::array<Voice, voiceCount> voices_ {};
    double hostSampleRate_ {44'100.0};
    std::uint64_t nextAge_ {1};
    double pitchBendRatio_ {1.0};
};

} // namespace e45recordings::play950::audio
