#pragma once

#include "audio/SampleVoicePool.h"
#include "formats/P9.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace e45recordings::play950::audio {

struct PreparedKeygroup {
    std::uint8_t lowKey {0};
    std::uint8_t highKey {0};
    std::optional<std::size_t> softSampleIndex;
    std::optional<std::size_t> loudSampleIndex;
    std::uint8_t velocityThreshold {128};
    std::int16_t softTuningSixteenths {0};
    std::int16_t loudTuningSixteenths {0};
    formats::P9Output output;
    bool oneShot {false};
    bool constantPitch {false};
    formats::P9Envelope amplitudeEnvelope {0, 0, 99, 0};
    formats::P9Envelope filterEnvelope {0, 0, 99, 0};
    std::int8_t filterEnvelopeAmount {0};
    std::uint8_t velocityToLoudness {0};
    std::uint8_t velocityToFilter {0};
    std::uint8_t keyboardToFilter {0};
    std::uint8_t softFilter {99};
    std::uint8_t loudFilter {99};
    std::int8_t softLoudness {0};
    std::int8_t loudLoudness {0};
    std::uint8_t midiChannelOffset {0};
};

struct PreparedProgram {
    std::vector<PreparedSample> samples;
    std::vector<PreparedKeygroup> keygroups;
};

class ProgramVoicePool {
public:
    static constexpr std::size_t voiceCount = 8;
    static constexpr std::size_t outputCount = 11;
    static constexpr std::size_t midiChannelCount = 16;
    using OutputBuffers = std::array<float*, outputCount>;

    void setProgram(PreparedProgram program);
    void setHostSampleRate(double sampleRate) noexcept;
    void setMidiReception(bool omni, int basicMidiChannel) noexcept;
    void noteOn(std::int32_t channel, std::int32_t pitch, std::int32_t noteId,
                float velocity) noexcept;
    void noteOn(std::int32_t pitch, std::int32_t noteId, float velocity) noexcept;
    void noteOff(std::int32_t channel, std::int32_t pitch, std::int32_t noteId) noexcept;
    void noteOff(std::int32_t pitch, std::int32_t noteId) noexcept;
    void setPitchBend(std::int32_t channel, float normalizedBipolar,
                      int rangeSemitones) noexcept;
    void setPitchBend(float normalizedBipolar, int rangeSemitones) noexcept;
    void renderAdd(const OutputBuffers& outputs, std::int32_t frameCount) noexcept;

    [[nodiscard]] std::size_t activeVoiceCount() const noexcept;
    [[nodiscard]] bool isNoteActive(std::int32_t channel, std::int32_t pitch,
                                    std::int32_t noteId) const noexcept;
    [[nodiscard]] bool isNoteActive(std::int32_t pitch, std::int32_t noteId) const noexcept;

private:
    static constexpr double monophonicHandoverMaximumSeconds = 0.001;
    static constexpr float monophonicHandoverNearZero = 1.0e-5F;

    struct EnvelopeState {
        enum class Stage { attack, decay, sustain, release, done };
        formats::P9Envelope parameters;
        Stage stage {Stage::done};
        float level {0.0F};
        float releaseStart {0.0F};
        std::uint64_t stageSamples {0};
    };

    struct Voice {
        double position {0.0};
        double increment {1.0};
        float gain {1.0F};
        std::uint64_t age {0};
        std::size_t sampleIndex {0};
        std::size_t keygroupIndex {0};
        formats::P9Output output;
        std::int32_t midiChannel {0};
        std::int32_t pitch {-1};
        std::int32_t noteId {-1};
        int playbackDirection {1};
        bool oneShot {false};
        bool active {false};
        float velocity {1.0F};
        float baseFilter {99.0F};
        float loudnessGain {1.0F};
        EnvelopeState amplitude;
        EnvelopeState filter;
        std::array<float, 4> filterState {};
    };

    struct PendingNote {
        std::int32_t midiChannel {0};
        std::int32_t pitch {-1};
        std::int32_t noteId {-1};
        float velocity {1.0F};
        std::size_t sampleIndex {0};
        std::size_t keygroupIndex {0};
        std::int16_t tuningSixteenths {0};
        bool useLoudLayer {false};
    };

    [[nodiscard]] Voice& voiceForNewNote() noexcept;
    void enforceOutputLimits(formats::P9Output output) noexcept;
    void initializeVoice(Voice& voice, const PendingNote& note) noexcept;
    void completeMonophonicHandover(std::size_t outputIndex, Voice& voice) noexcept;
    float nextSample(Voice& voice) noexcept;
    float advanceEnvelope(EnvelopeState& envelope) const noexcept;

    PreparedProgram program_;
    std::array<Voice, voiceCount> voices_ {};
    std::array<std::optional<PendingNote>, 8> pendingMonophonicNotes_ {};
    std::array<float, 8> previousMonophonicOutputs_ {};
    std::array<std::uint64_t, 8> monophonicHandoverSamples_ {};
    double hostSampleRate_ {44'100.0};
    std::uint64_t nextAge_ {1};
    std::array<double, midiChannelCount> pitchBendRatios_ {
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    bool midiOmni_ {true};
    std::int32_t basicMidiChannel_ {1};
};

} // namespace e45recordings::play950::audio
