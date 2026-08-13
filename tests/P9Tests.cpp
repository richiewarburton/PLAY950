#include "formats/P9.h"

#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace e45recordings::play950::formats;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<std::byte> minimalP9() {
    std::vector<std::byte> data(P9Program::headerSize + P9Keygroup::recordSize);
    constexpr std::string_view name = "TEST P9";
    for (std::size_t i = 0; i < name.size(); ++i)
        data[i] = static_cast<std::byte>(name[i]);
    data[0x17] = std::byte {1};
    const auto base = P9Program::headerSize;
    data[base + 0x00] = std::byte {127};
    data[base + 0x01] = std::byte {0};
    data[base + 0x02] = std::byte {128};
    data[base + 0x08] = std::byte {50};
    data[base + 0x12] = std::byte {0x3b};
    data[base + 0x13] = std::byte {0xff};
    data[base + 0x14] = std::byte {15};
    data[base + 0x2c] = std::byte {99};
    data[base + 0x42] = std::byte {99};
    return data;
}

void unitTests() {
    auto bytes = minimalP9();
    const auto parsed = parseP9(bytes);
    require(parsed.name == "TEST P9", "program name decoding failed");
    require(parsed.keygroups.size() == 1, "keygroup count decoding failed");
    const auto& keygroup = parsed.keygroups.front();
    require(keygroup.lowKey == 0 && keygroup.highKey == 127,
            "key range decoding failed");
    require(keygroup.keyboardToFilter == 50,
            "keyboard-to-filter decoding failed");
    require(keygroup.constantPitch && keygroup.velocityCrossfade && keygroup.oneShot &&
                keygroup.releaseVelocityFromNoteOn && keygroup.customVelocityCrossfadePoint,
            "known flag decoding failed");
    require(keygroup.output.kind() == P9OutputKind::all &&
                keygroup.output.displayedNumber() == 0,
            "All output decoding failed");
    require(keygroup.midiChannelOffset == 15,
            "MIDI channel offset decoding failed");

    auto placeholderProgram = parsed;
    placeholderProgram.keygroups.front().loudSampleName = "2 SAMPLE";
    const auto placeholderResolved = resolveP9Samples(placeholderProgram, {});
    require(!placeholderResolved.front().loudSampleIndex,
            "absent default sample placeholder was not ignored");
    require(isDefaultSamplePlaceholder("2 sample") &&
                !isDefaultSamplePlaceholder("2 SAMPLE X"),
            "default sample placeholder recognition failed");

    placeholderProgram.keygroups.front().softSampleName = "2 SAMPLE";
    const auto softPlaceholderResolved = resolveP9Samples(placeholderProgram, {});
    require(!softPlaceholderResolved.front().softSampleIndex,
            "absent Soft placeholder was not ignored");

    placeholderProgram.keygroups.front().softSampleName = "MISSING01";
    placeholderProgram.keygroups.front().loudSampleName = "MISSING02";
    const auto missingResolved = resolveP9Samples(placeholderProgram, {});
    require(!missingResolved.front().softSampleIndex &&
                !missingResolved.front().loudSampleIndex,
            "missing linked samples did not resolve to silent layers");

    bytes.pop_back();
    try {
        (void)parseP9(bytes);
        throw std::runtime_error("truncated P9 was accepted");
    } catch (const P9ParseError&) {
    }

    P9Tuning positive {380};
    require(positive.transpose() == 24 && positive.fine() == -4,
            "nearest-semitone positive tuning failed");
    P9Tuning negative {-380};
    require(negative.transpose() == -24 && negative.fine() == 4,
            "nearest-semitone negative tuning failed");
}

} // namespace

int main() {
    try {
        unitTests();
        std::cout << "PLAY950 P9 tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PLAY950 P9 tests failed: " << error.what() << '\n';
        return 1;
    }
}
