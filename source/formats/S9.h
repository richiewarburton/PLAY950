#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace e45recordings::play950::formats {

enum class S9PlaybackMode : std::uint8_t {
    oneShot = 0x4f,
    loop = 0x4c,
    alternatingLoop = 0x41,
};

enum class S9Direction : std::uint8_t {
    forward = 0x4e,
    reverse = 0x52,
};

struct S9Sample {
    static constexpr std::size_t headerSize = 0x3c;

    std::array<std::byte, headerSize> rawHeader {};
    std::string name;
    std::uint32_t sampleCount {0};
    std::uint16_t sampleRate {0};
    std::uint16_t nominalPitchSixteenths {0};
    std::int16_t loudnessOffset {0};
    S9PlaybackMode playbackMode {S9PlaybackMode::oneShot};
    std::uint32_t playbackEnd {0};
    std::uint32_t playbackStart {0};
    std::uint32_t loopLength {0};
    std::uint8_t sampleType {0};
    S9Direction direction {S9Direction::forward};
    std::vector<std::int16_t> samples12;

    [[nodiscard]] std::uint16_t rootNote() const noexcept {
        return nominalPitchSixteenths / 16;
    }
    [[nodiscard]] std::uint16_t finePitchSixteenths() const noexcept {
        return nominalPitchSixteenths % 16;
    }
    [[nodiscard]] std::uint32_t loopStart() const noexcept {
        return loopLength <= playbackEnd ? playbackEnd - loopLength : 0;
    }
};

class S9ParseError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] std::size_t s9PackedPayloadSize(std::uint32_t sampleCount) noexcept;
[[nodiscard]] S9Sample parseUncompressedS9(std::span<const std::byte> fileData);

} // namespace e45recordings::play950::formats
