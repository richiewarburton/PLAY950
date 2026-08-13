#include "state/ProjectState.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace e45recordings::play950::state;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

void requireRejected(std::vector<std::byte> bytes, const std::string& message) {
    try {
        (void)deserializeProjectState(bytes);
        throw std::runtime_error(message);
    } catch (const ProjectStateError&) {
    }
}

void appendU32(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
}

void appendBlob(std::vector<std::byte>& bytes, const std::vector<std::byte>& blob) {
    appendU32(bytes, static_cast<std::uint32_t>(blob.size()));
    bytes.insert(bytes.end(), blob.begin(), blob.end());
}

std::vector<std::byte> legacyState(const ProjectState& state) {
    std::vector<std::byte> bytes {
        std::byte {'T'}, std::byte {'R'}, std::byte {'U'}, std::byte {'E'},
        std::byte {'9'}, std::byte {'5'}, std::byte {'0'}, std::byte {0}};
    appendU32(bytes, 1);
    appendU32(bytes, static_cast<std::uint32_t>(state.s9Samples.size()));
    appendBlob(bytes, state.p9);
    for (const auto& sample : state.s9Samples)
        appendBlob(bytes, sample);
    return bytes;
}

std::vector<std::byte> version2State(const ProjectState& state) {
    auto bytes = serializeProjectState(state);
    bytes[8] = std::byte {2};
    // v3 inserts sourcePath immediately after sourceName. This helper is used
    // with an empty path, whose encoding is one zero u32.
    std::size_t position = 12;
    const auto readU32 = [&](std::size_t at) {
        std::uint32_t value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8)
            value |= std::to_integer<std::uint32_t>(bytes[at + shift / 8]) << shift;
        return value;
    };
    const auto sampleCount = readU32(position); position += 4;
    position += 4 + readU32(position);
    for (std::uint32_t index = 0; index < sampleCount; ++index)
        position += 4 + readU32(position);
    position += 4 + readU32(position); // sourceName
    bytes.erase(bytes.begin() + static_cast<std::ptrdiff_t>(position),
                bytes.begin() + static_cast<std::ptrdiff_t>(position + 4));
    // v4 appends pitch-bend range and v5 appends MIDI receive mode/channel.
    bytes.resize(bytes.size() - 12);
    return bytes;
}

std::vector<std::byte> version4State(const ProjectState& state) {
    auto bytes = serializeProjectState(state);
    bytes[8] = std::byte {4};
    bytes.resize(bytes.size() - 8);
    return bytes;
}

} // namespace

int main() {
    try {
        ProjectState original;
        original.p9 = {std::byte {0x10}, std::byte {0x20}, std::byte {0x30}};
        original.s9Samples = {{std::byte {0x01}, std::byte {0x02}},
                              {std::byte {0xfe}, std::byte {0xff}}};
        original.pitchBendRangeSemitones = 12;
        original.midiOmni = false;
        original.basicMidiChannel = 9;
        const auto encoded = serializeProjectState(original);
        require(deserializeProjectState(encoded) == original, "project-state round trip failed");

        ProjectState browserState;
        browserState.sourceName = "MULTI.img";
        browserState.sourcePath = "/Volumes/AKAI/MULTI.img";
        browserState.browserPrograms = {
            {"ADG.P9", {std::byte {0x01}}, {{std::byte {0x11}}}},
            {"ADGPASTE.P9", {std::byte {0x02}}, {{std::byte {0x22}, std::byte {0x23}}}},
            {"ADGPASTE2.P9", {std::byte {0x03}}, {{std::byte {0x33}}}}};
        browserState.selectedProgramIndex = 1;
        browserState.p9 = browserState.browserPrograms[1].p9;
        browserState.s9Samples = browserState.browserPrograms[1].s9Samples;
        const auto browserEncoded = serializeProjectState(browserState);
        require(deserializeProjectState(browserEncoded) == browserState,
                "multi-program project-state round trip failed");

        auto version4Browser = browserState;
        version4Browser.pitchBendRangeSemitones = 7;
        version4Browser.midiOmni = false;
        version4Browser.basicMidiChannel = 12;
        auto migratedVersion4 = deserializeProjectState(version4State(version4Browser));
        version4Browser.midiOmni = true;
        version4Browser.basicMidiChannel = 1;
        require(migratedVersion4 == version4Browser,
                "version-4 MIDI defaults were not restored");

        auto version2Browser = browserState;
        version2Browser.sourcePath.clear();
        const auto migratedCollection = deserializeProjectState(version2State(version2Browser));
        require(migratedCollection == version2Browser,
                "version-2 collection state was not restored");

        const auto migrated = deserializeProjectState(legacyState(original));
        require(migrated.p9 == original.p9 && migrated.s9Samples == original.s9Samples,
                "legacy project-state payload was not restored");
        require(migrated.browserPrograms.empty() && migrated.sourceName.empty() &&
                    migrated.selectedProgramIndex == 0,
                "legacy project state acquired unexpected browser metadata");

        const auto emptyEncoded = serializeProjectState({});
        require(deserializeProjectState(emptyEncoded) == ProjectState {},
                "empty project-state round trip failed");

        auto badMagic = encoded;
        badMagic[0] = std::byte {0};
        requireRejected(std::move(badMagic), "invalid signature was accepted");

        auto badVersion = encoded;
        badVersion[8] = std::byte {99};
        requireRejected(std::move(badVersion), "unknown version was accepted");

        auto truncated = encoded;
        truncated.pop_back();
        requireRejected(std::move(truncated), "truncated state was accepted");

        auto trailing = encoded;
        trailing.push_back(std::byte {0});
        requireRejected(std::move(trailing), "trailing data was accepted");

        auto badOmni = encoded;
        badOmni[badOmni.size() - 8] = std::byte {2};
        requireRejected(std::move(badOmni), "invalid MIDI Omni setting was accepted");

        auto badBasicChannel = encoded;
        badBasicChannel[badBasicChannel.size() - 4] = std::byte {17};
        requireRejected(std::move(badBasicChannel), "invalid basic MIDI channel was accepted");

        std::cout << "PLAY950 project-state tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PLAY950 project-state tests failed: " << error.what() << '\n';
        return 1;
    }
}
