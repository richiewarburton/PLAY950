#pragma once

#include "formats/S9.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace e45recordings::play950::formats {

struct P9Envelope {
    std::uint8_t attack {0};
    std::uint8_t decay {0};
    std::uint8_t sustain {0};
    std::uint8_t release {0};
};

struct P9Tuning {
    std::int16_t rawSixteenths {0};
    [[nodiscard]] int transpose() const noexcept;
    [[nodiscard]] int fine() const noexcept;
};

enum class P9OutputKind { all, mono, left, right, unknown };

struct P9Output {
    std::uint8_t raw {0xff};
    [[nodiscard]] P9OutputKind kind() const noexcept;
    [[nodiscard]] int displayedNumber() const noexcept;
};

struct P9Keygroup {
    static constexpr std::size_t recordSize = 0x46;
    std::array<std::byte, recordSize> rawRecord {};
    std::uint8_t highKey {0};
    std::uint8_t lowKey {0};
    std::uint8_t velocityThreshold {0};
    P9Envelope amplitudeEnvelope;
    std::uint8_t velocityToFilter {0};
    std::uint8_t velocityToAttack {0};
    std::int8_t velocityToRelease {0};
    std::uint8_t velocityToLoudness {0};
    std::uint8_t keyboardToFilter {0};
    std::uint8_t flags {0};
    bool constantPitch {false};
    bool velocityCrossfade {false};
    bool oneShot {false};
    bool releaseVelocityFromNoteOn {false};
    bool customVelocityCrossfadePoint {false};
    P9Output output;
    std::uint8_t midiChannelOffset {0};
    std::int8_t vcfAmount {0};
    std::string softSampleName;
    P9Envelope vcfEnvelope;
    std::uint8_t velocityCrossfadePoint {0};
    P9Tuning softTuning;
    std::uint8_t softFilter {0};
    std::int8_t softLoudness {0};
    std::string loudSampleName;
    P9Tuning loudTuning;
    std::uint8_t loudFilter {0};
    std::int8_t loudLoudness {0};
};

struct P9Program {
    static constexpr std::size_t headerSize = 0x26;
    std::array<std::byte, headerSize> rawHeader {};
    std::string name;
    bool positionalCrossfade {false};
    std::vector<P9Keygroup> keygroups;
};

struct P9ResolvedKeygroup {
    std::optional<std::size_t> softSampleIndex;
    std::optional<std::size_t> loudSampleIndex;
};

class P9ParseError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] P9Program parseP9(std::span<const std::byte> fileData);
[[nodiscard]] bool isDefaultSamplePlaceholder(std::string_view name) noexcept;
[[nodiscard]] std::vector<P9ResolvedKeygroup> resolveP9Samples(
    const P9Program& program, std::span<const S9Sample> samples);

} // namespace e45recordings::play950::formats
