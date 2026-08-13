#include "formats/S9.h"

#include <algorithm>
#include <limits>

namespace e45recordings::play950::formats {
namespace {

std::uint8_t byteAt(std::span<const std::byte> data, std::size_t offset) {
    return std::to_integer<std::uint8_t>(data[offset]);
}

std::uint16_t readU16(std::span<const std::byte> data, std::size_t offset) {
    return static_cast<std::uint16_t>(byteAt(data, offset)) |
           static_cast<std::uint16_t>(byteAt(data, offset + 1) << 8U);
}

std::uint32_t readU32(std::span<const std::byte> data, std::size_t offset) {
    return static_cast<std::uint32_t>(byteAt(data, offset)) |
           (static_cast<std::uint32_t>(byteAt(data, offset + 1)) << 8U) |
           (static_cast<std::uint32_t>(byteAt(data, offset + 2)) << 16U) |
           (static_cast<std::uint32_t>(byteAt(data, offset + 3)) << 24U);
}

std::int16_t decode12(std::uint8_t upperEightBits, std::uint8_t lowerFourBits) {
    const std::uint16_t word = static_cast<std::uint16_t>(upperEightBits) << 8U |
                               static_cast<std::uint16_t>(lowerFourBits & 0x0fU) << 4U;
    const std::int32_t signedWord = (word & 0x8000U) != 0U
                                        ? static_cast<std::int32_t>(word) - 0x10000
                                        : static_cast<std::int32_t>(word);
    return static_cast<std::int16_t>(signedWord / 16);
}

template <typename Enum>
Enum checkedEnum(std::uint8_t raw, std::initializer_list<Enum> valid, const char* field) {
    const auto value = static_cast<Enum>(raw);
    if (std::find(valid.begin(), valid.end(), value) == valid.end())
        throw S9ParseError(std::string("Unsupported S9 ") + field + " value");
    return value;
}

} // namespace

std::size_t s9PackedPayloadSize(std::uint32_t sampleCount) noexcept {
    return static_cast<std::size_t>(3) * ((static_cast<std::size_t>(sampleCount) + 1) / 2);
}

S9Sample parseUncompressedS9(std::span<const std::byte> fileData) {
    if (fileData.size() < S9Sample::headerSize)
        throw S9ParseError("S9 file is shorter than its 60-byte header");

    S9Sample result;
    std::copy_n(fileData.begin(), S9Sample::headerSize, result.rawHeader.begin());

    for (std::size_t i = 0; i < 10; ++i) {
        const auto character = byteAt(fileData, i);
        if (character == 0) {
            result.name.push_back(' ');
            continue;
        }
        if (character < 0x20 || character > 0x7e)
            throw S9ParseError("S9 name contains a non-printable byte");
        result.name.push_back(static_cast<char>(character));
    }
    while (!result.name.empty() && result.name.back() == ' ')
        result.name.pop_back();

    result.sampleCount = readU32(fileData, 0x10);
    result.sampleRate = readU16(fileData, 0x14);
    result.nominalPitchSixteenths = readU16(fileData, 0x16);
    result.loudnessOffset = static_cast<std::int16_t>(readU16(fileData, 0x18));
    result.playbackMode = checkedEnum<S9PlaybackMode>(
        byteAt(fileData, 0x1a),
        {S9PlaybackMode::oneShot, S9PlaybackMode::loop, S9PlaybackMode::alternatingLoop},
        "playback mode");
    result.playbackEnd = readU32(fileData, 0x1c);
    result.playbackStart = readU32(fileData, 0x20);
    result.loopLength = readU32(fileData, 0x24);
    result.sampleType = byteAt(fileData, 0x2a);
    result.direction = checkedEnum<S9Direction>(
        byteAt(fileData, 0x2b), {S9Direction::forward, S9Direction::reverse}, "direction");

    if (result.sampleCount == 0)
        throw S9ParseError("S9 sample contains no audio");
    if (result.sampleRate < 7'000 || result.sampleRate > 50'000)
        throw S9ParseError("S9 sample rate is outside the supported S950 range");
    if (result.rootNote() > 127)
        throw S9ParseError("S9 nominal pitch is outside the MIDI note range");
    if (result.playbackStart > result.sampleCount || result.playbackEnd > result.sampleCount ||
        result.playbackStart > result.playbackEnd || result.loopLength > result.playbackEnd)
        throw S9ParseError("S9 playback or loop markers are inconsistent");

    const auto payloadSize = s9PackedPayloadSize(result.sampleCount);
    if (fileData.size() != S9Sample::headerSize + payloadSize)
        throw S9ParseError("S9 payload size does not match uncompressed 12-bit sample count");

    const auto payload = fileData.subspan(S9Sample::headerSize);
    const std::size_t firstHalfCount = (static_cast<std::size_t>(result.sampleCount) + 1) / 2;
    const std::size_t secondHalfCount = result.sampleCount - firstHalfCount;
    result.samples12.resize(result.sampleCount);

    for (std::size_t i = 0; i < firstHalfCount; ++i) {
        const auto sharedNibbles = byteAt(payload, i * 2);
        result.samples12[i] = decode12(byteAt(payload, i * 2 + 1), sharedNibbles >> 4U);
    }
    const std::size_t secondPlane = firstHalfCount * 2;
    for (std::size_t i = 0; i < secondHalfCount; ++i) {
        const auto sharedNibbles = byteAt(payload, i * 2);
        result.samples12[firstHalfCount + i] =
            decode12(byteAt(payload, secondPlane + i), sharedNibbles & 0x0fU);
    }
    return result;
}

} // namespace e45recordings::play950::formats
