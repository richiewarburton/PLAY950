#include "formats/P9.h"

#include <algorithm>
#include <cctype>

namespace e45recordings::play950::formats {
namespace {

std::uint8_t byteAt(std::span<const std::byte> data, std::size_t offset) {
    return std::to_integer<std::uint8_t>(data[offset]);
}

std::int16_t readS16(std::span<const std::byte> data, std::size_t offset) {
    const auto raw = static_cast<std::uint16_t>(byteAt(data, offset)) |
                     static_cast<std::uint16_t>(byteAt(data, offset + 1) << 8U);
    return static_cast<std::int16_t>(raw);
}

std::string decodeName(std::span<const std::byte> data) {
    std::string name;
    name.reserve(data.size());
    for (const auto value : data) {
        const auto character = std::to_integer<std::uint8_t>(value);
        if (character == 0) {
            name.push_back(' ');
        } else if (character >= 0x20 && character <= 0x7e) {
            name.push_back(static_cast<char>(character));
        } else {
            throw P9ParseError("P9 name contains a non-printable byte");
        }
    }
    while (!name.empty() && name.back() == ' ')
        name.pop_back();
    return name;
}

std::string nameKey(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char value) {
        return static_cast<char>(std::toupper(value));
    });
    return name;
}

P9Envelope envelopeAt(std::span<const std::byte> record, std::size_t offset) {
    return {byteAt(record, offset), byteAt(record, offset + 1),
            byteAt(record, offset + 2), byteAt(record, offset + 3)};
}

P9Keygroup parseKeygroup(std::span<const std::byte> record) {
    P9Keygroup result;
    std::copy_n(record.begin(), P9Keygroup::recordSize, result.rawRecord.begin());
    result.highKey = byteAt(record, 0x00);
    result.lowKey = byteAt(record, 0x01);
    result.velocityThreshold = byteAt(record, 0x02);
    result.amplitudeEnvelope = envelopeAt(record, 0x03);
    result.velocityToFilter = byteAt(record, 0x07);
    result.velocityToAttack = byteAt(record, 0x09);
    result.velocityToRelease = static_cast<std::int8_t>(byteAt(record, 0x0a));
    result.velocityToLoudness = byteAt(record, 0x0b);
    result.keyboardToFilter = byteAt(record, 0x08);
    result.flags = byteAt(record, 0x12);
    result.constantPitch = (result.flags & 0x01U) != 0;
    result.velocityCrossfade = (result.flags & 0x02U) != 0;
    result.oneShot = (result.flags & 0x08U) != 0;
    result.releaseVelocityFromNoteOn = (result.flags & 0x10U) != 0;
    result.customVelocityCrossfadePoint = (result.flags & 0x20U) != 0;
    result.output.raw = byteAt(record, 0x13);
    result.midiChannelOffset = byteAt(record, 0x14);
    result.vcfAmount = static_cast<std::int8_t>(byteAt(record, 0x17));
    result.softSampleName = decodeName(record.subspan(0x18, 10));
    result.vcfEnvelope = envelopeAt(record, 0x22);
    result.velocityCrossfadePoint = byteAt(record, 0x26);
    result.softTuning.rawSixteenths = readS16(record, 0x2a);
    result.softFilter = byteAt(record, 0x2c);
    result.softLoudness = static_cast<std::int8_t>(byteAt(record, 0x2d));
    result.loudSampleName = decodeName(record.subspan(0x2e, 10));
    result.loudTuning.rawSixteenths = readS16(record, 0x40);
    result.loudFilter = byteAt(record, 0x42);
    result.loudLoudness = static_cast<std::int8_t>(byteAt(record, 0x43));

    if (result.lowKey > result.highKey || result.highKey > 127)
        throw P9ParseError("P9 keygroup has an invalid key range");
    if (result.velocityThreshold > 128)
        throw P9ParseError("P9 keygroup has an invalid velocity threshold");
    if (result.midiChannelOffset > 15)
        throw P9ParseError("P9 keygroup has an invalid MIDI channel");
    return result;
}

} // namespace

int P9Tuning::transpose() const noexcept {
    const int adjusted = static_cast<int>(rawSixteenths) + 8;
    return adjusted >= 0 ? adjusted / 16 : -((-adjusted + 15) / 16);
}

int P9Tuning::fine() const noexcept {
    return static_cast<int>(rawSixteenths) - transpose() * 16;
}

P9OutputKind P9Output::kind() const noexcept {
    if (raw == 0xff) return P9OutputKind::all;
    if (raw <= 0x07) return P9OutputKind::mono;
    if (raw == 0x08) return P9OutputKind::left;
    if (raw == 0x09) return P9OutputKind::right;
    return P9OutputKind::unknown;
}

int P9Output::displayedNumber() const noexcept {
    switch (kind()) {
        case P9OutputKind::all: return 0;
        case P9OutputKind::mono: return static_cast<int>(raw) + 1;
        case P9OutputKind::left: return 9;
        case P9OutputKind::right: return 10;
        case P9OutputKind::unknown: return -1;
    }
    return -1;
}

P9Program parseP9(std::span<const std::byte> fileData) {
    if (fileData.size() < P9Program::headerSize)
        throw P9ParseError("P9 file is shorter than its 38-byte header");
    const auto keygroupCount = byteAt(fileData, 0x17);
    if (keygroupCount == 0 || keygroupCount > 99)
        throw P9ParseError("P9 keygroup count is outside 1...99");
    const auto expectedSize = P9Program::headerSize +
                              static_cast<std::size_t>(keygroupCount) * P9Keygroup::recordSize;
    if (fileData.size() != expectedSize)
        throw P9ParseError("P9 file size does not match its keygroup count");

    P9Program result;
    std::copy_n(fileData.begin(), P9Program::headerSize, result.rawHeader.begin());
    result.name = decodeName(fileData.first(10));
    result.positionalCrossfade = byteAt(fileData, 0x15) != 0;
    result.keygroups.reserve(keygroupCount);
    for (std::size_t index = 0; index < keygroupCount; ++index) {
        const auto offset = P9Program::headerSize + index * P9Keygroup::recordSize;
        result.keygroups.push_back(parseKeygroup(fileData.subspan(offset, P9Keygroup::recordSize)));
    }
    return result;
}

bool isDefaultSamplePlaceholder(std::string_view name) noexcept {
    if (name.size() != 8)
        return false;
    constexpr std::string_view placeholder = "2 SAMPLE";
    for (std::size_t index = 0; index < name.size(); ++index) {
        if (std::toupper(static_cast<unsigned char>(name[index])) != placeholder[index])
            return false;
    }
    return true;
}

std::vector<P9ResolvedKeygroup> resolveP9Samples(
    const P9Program& program, std::span<const S9Sample> samples) {
    std::vector<P9ResolvedKeygroup> result(program.keygroups.size());
    auto resolve = [&](const std::string& requested) -> std::optional<std::size_t> {
        if (requested.empty() || isDefaultSamplePlaceholder(requested))
            return std::nullopt;
        const auto key = nameKey(requested);
        std::optional<std::size_t> found;
        for (std::size_t index = 0; index < samples.size(); ++index) {
            if (nameKey(samples[index].name) == key) {
                if (found)
                    throw P9ParseError("P9 sample reference is ambiguous: " + requested);
                found = index;
            }
        }
        // Missing sample data is valid: the voice pool treats this unresolved layer as silence.
        return found;
    };
    for (std::size_t index = 0; index < program.keygroups.size(); ++index) {
        result[index].softSampleIndex = resolve(program.keygroups[index].softSampleName);
        result[index].loudSampleIndex = resolve(program.keygroups[index].loudSampleName);
    }
    return result;
}

} // namespace e45recordings::play950::formats
