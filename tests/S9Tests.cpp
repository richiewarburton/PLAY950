#include "formats/S9.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using e45recordings::play950::formats::S9Direction;
using e45recordings::play950::formats::S9PlaybackMode;
using e45recordings::play950::formats::S9Sample;
using e45recordings::play950::formats::parseUncompressedS9;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

void writeU16(std::vector<std::byte>& data, std::size_t offset, std::uint16_t value) {
    data[offset] = static_cast<std::byte>(value & 0xffU);
    data[offset + 1] = static_cast<std::byte>(value >> 8U);
}

void writeU32(std::vector<std::byte>& data, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        data[offset + i] = static_cast<std::byte>((value >> (i * 8U)) & 0xffU);
}

std::vector<std::byte> makeSynthetic() {
    constexpr std::array<std::int16_t, 5> samples {-2048, -1, 0, 1, 2047};
    constexpr std::size_t half = 3;
    std::vector<std::byte> file(S9Sample::headerSize + half * 3);
    const std::string name = "PACKTEST";
    for (std::size_t i = 0; i < name.size(); ++i)
        file[i] = static_cast<std::byte>(name[i]);
    for (std::size_t i = name.size(); i < 10; ++i)
        file[i] = std::byte {0x20};
    writeU32(file, 0x10, samples.size());
    writeU16(file, 0x14, 44'100);
    writeU16(file, 0x16, 60 * 16 + 7);
    file[0x1a] = std::byte {0x4f};
    writeU32(file, 0x1c, samples.size());
    file[0x2b] = std::byte {0x4e};

    auto encodeParts = [](std::int16_t sample) {
        const auto raw = static_cast<std::uint16_t>(static_cast<std::int32_t>(sample) * 16);
        return std::pair {static_cast<std::uint8_t>(raw >> 8U),
                          static_cast<std::uint8_t>((raw >> 4U) & 0x0fU)};
    };
    auto* payload = file.data() + S9Sample::headerSize;
    for (std::size_t i = 0; i < half; ++i) {
        const auto [upper, lower] = encodeParts(samples[i]);
        payload[i * 2] = static_cast<std::byte>(lower << 4U);
        payload[i * 2 + 1] = static_cast<std::byte>(upper);
    }
    for (std::size_t i = 0; i < samples.size() - half; ++i) {
        const auto [upper, lower] = encodeParts(samples[half + i]);
        payload[i * 2] |= static_cast<std::byte>(lower);
        payload[half * 2 + i] = static_cast<std::byte>(upper);
    }
    return file;
}

void unitTests() {
    const auto synthetic = makeSynthetic();
    const auto parsed = parseUncompressedS9(synthetic);
    require(parsed.name == "PACKTEST", "name decoding failed");
    require(parsed.sampleRate == 44'100, "sample-rate decoding failed");
    require(parsed.rootNote() == 60 && parsed.finePitchSixteenths() == 7,
            "nominal-pitch decoding failed");
    require(parsed.samples12 == std::vector<std::int16_t>({-2048, -1, 0, 1, 2047}),
            "12-bit plane/nibble decoding failed");

    auto truncated = synthetic;
    truncated.pop_back();
    try {
        (void)parseUncompressedS9(truncated);
        throw std::runtime_error("truncated payload was accepted");
    } catch (const e45recordings::play950::formats::S9ParseError&) {
    }
}

} // namespace

int main() {
    try {
        unitTests();
        std::cout << "PLAY950 S9 tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PLAY950 S9 tests failed: " << error.what() << '\n';
        return 1;
    }
}
