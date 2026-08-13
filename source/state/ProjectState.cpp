#include "state/ProjectState.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace e45recordings::play950::state {
namespace {

constexpr std::array<std::byte, 8> magic {
    std::byte {'T'}, std::byte {'R'}, std::byte {'U'}, std::byte {'E'},
    std::byte {'9'}, std::byte {'5'}, std::byte {'0'}, std::byte {0}};
constexpr std::uint32_t legacyFormatVersion = 1;
constexpr std::uint32_t collectionFormatVersion = 2;
constexpr std::uint32_t pathFormatVersion = 3;
constexpr std::uint32_t pitchBendFormatVersion = 4;
constexpr std::uint32_t formatVersion = 5;
constexpr std::size_t maximumStateBytes = 64U * 1024U * 1024U;
constexpr std::uint32_t maximumSampleCount = 256;
constexpr std::uint32_t maximumProgramCount = 256;
constexpr std::size_t maximumNameBytes = 4096;

void appendU32(std::vector<std::byte>& output, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8)
        output.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
}

void appendBlob(std::vector<std::byte>& output, std::span<const std::byte> bytes) {
    if (bytes.size() > std::numeric_limits<std::uint32_t>::max())
        throw ProjectStateError("project-state blob is too large");
    appendU32(output, static_cast<std::uint32_t>(bytes.size()));
    output.insert(output.end(), bytes.begin(), bytes.end());
}

void appendString(std::vector<std::byte>& output, const std::string& value) {
    if (value.size() > maximumNameBytes)
        throw ProjectStateError("project-state name is too long");
    appendBlob(output, std::as_bytes(std::span(value)));
}

void validateProgram(std::span<const std::byte> p9,
                     const std::vector<std::vector<std::byte>>& samples) {
    if (samples.size() > maximumSampleCount)
        throw ProjectStateError("too many samples in project state");
    if (p9.empty() && !samples.empty())
        throw ProjectStateError("sample data requires a P9 program");
}

void appendProgram(std::vector<std::byte>& output, std::span<const std::byte> p9,
                   const std::vector<std::vector<std::byte>>& samples) {
    validateProgram(p9, samples);
    appendU32(output, static_cast<std::uint32_t>(samples.size()));
    appendBlob(output, p9);
    for (const auto& sample : samples)
        appendBlob(output, sample);
}

class Reader {
public:
    explicit Reader(std::span<const std::byte> bytes) : bytes_(bytes) {}

    std::uint32_t u32() {
        if (remaining() < 4)
            throw ProjectStateError("truncated project-state integer");
        std::uint32_t value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8)
            value |= std::to_integer<std::uint32_t>(bytes_[position_++]) << shift;
        return value;
    }

    std::vector<std::byte> blob() {
        const auto size = static_cast<std::size_t>(u32());
        if (size > remaining())
            throw ProjectStateError("truncated project-state blob");
        std::vector<std::byte> result(bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
                                      bytes_.begin() + static_cast<std::ptrdiff_t>(position_ + size));
        position_ += size;
        return result;
    }

    std::string string() {
        const auto bytes = blob();
        if (bytes.size() > maximumNameBytes)
            throw ProjectStateError("project-state name is too long");
        if (bytes.empty())
            return {};
        return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
    }

    [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - position_; }

private:
    std::span<const std::byte> bytes_;
    std::size_t position_ {0};
};

EmbeddedProgram readProgram(Reader& reader) {
    const auto sampleCount = reader.u32();
    if (sampleCount > maximumSampleCount)
        throw ProjectStateError("too many samples in project state");
    EmbeddedProgram result;
    result.p9 = reader.blob();
    result.s9Samples.reserve(sampleCount);
    for (std::uint32_t index = 0; index < sampleCount; ++index)
        result.s9Samples.push_back(reader.blob());
    validateProgram(result.p9, result.s9Samples);
    return result;
}

} // namespace

std::vector<std::byte> serializeProjectState(const ProjectState& state) {
    if (state.pitchBendRangeSemitones < 1 || state.pitchBendRangeSemitones > 12)
        throw ProjectStateError("pitch-bend range is outside 1-12 semitones");
    if (state.basicMidiChannel < 1 || state.basicMidiChannel > 16)
        throw ProjectStateError("basic MIDI channel is outside 1-16");
    validateProgram(state.p9, state.s9Samples);
    if (state.browserPrograms.size() > maximumProgramCount)
        throw ProjectStateError("too many programs in project state");
    if (state.browserPrograms.empty() && state.selectedProgramIndex != 0)
        throw ProjectStateError("selected program requires a browser collection");
    if (!state.browserPrograms.empty() &&
        state.selectedProgramIndex >= state.browserPrograms.size())
        throw ProjectStateError("selected program index is out of range");
    for (const auto& program : state.browserPrograms) {
        if (program.fileName.size() > maximumNameBytes)
            throw ProjectStateError("project-state filename is too long");
        if (program.p9.empty())
            throw ProjectStateError("browser program is missing its P9 data");
        validateProgram(program.p9, program.s9Samples);
    }
    if (!state.browserPrograms.empty()) {
        const auto& selected = state.browserPrograms[state.selectedProgramIndex];
        if (state.p9 != selected.p9 || state.s9Samples != selected.s9Samples)
            throw ProjectStateError("active program does not match the selected browser program");
    }

    std::vector<std::byte> output;
    output.reserve(20 + state.p9.size());
    output.insert(output.end(), magic.begin(), magic.end());
    appendU32(output, formatVersion);
    appendProgram(output, state.p9, state.s9Samples);
    appendString(output, state.sourceName);
    appendString(output, state.sourcePath);
    appendU32(output, state.selectedProgramIndex);
    appendU32(output, static_cast<std::uint32_t>(state.browserPrograms.size()));
    for (const auto& program : state.browserPrograms) {
        appendString(output, program.fileName);
        appendProgram(output, program.p9, program.s9Samples);
    }
    appendU32(output, state.pitchBendRangeSemitones);
    appendU32(output, state.midiOmni ? 1U : 0U);
    appendU32(output, state.basicMidiChannel);
    if (output.size() > maximumStateBytes)
        throw ProjectStateError("project state exceeds the size limit");
    return output;
}

ProjectState deserializeProjectState(std::span<const std::byte> bytes) {
    if (bytes.size() > maximumStateBytes)
        throw ProjectStateError("project state exceeds the size limit");
    if (bytes.size() < magic.size() || !std::equal(magic.begin(), magic.end(), bytes.begin()))
        throw ProjectStateError("invalid project-state signature");

    Reader reader(bytes.subspan(magic.size()));
    const auto version = reader.u32();
    if (version != legacyFormatVersion && version != collectionFormatVersion &&
        version != pathFormatVersion && version != pitchBendFormatVersion &&
        version != formatVersion)
        throw ProjectStateError("unsupported project-state version");
    auto active = readProgram(reader);
    ProjectState result;
    result.p9 = std::move(active.p9);
    result.s9Samples = std::move(active.s9Samples);
    if (version == collectionFormatVersion || version == pathFormatVersion ||
        version == pitchBendFormatVersion || version == formatVersion) {
        result.sourceName = reader.string();
        if (version == pathFormatVersion || version == pitchBendFormatVersion ||
            version == formatVersion)
            result.sourcePath = reader.string();
        result.selectedProgramIndex = reader.u32();
        const auto programCount = reader.u32();
        if (programCount > maximumProgramCount)
            throw ProjectStateError("too many programs in project state");
        result.browserPrograms.reserve(programCount);
        for (std::uint32_t index = 0; index < programCount; ++index) {
            const auto fileName = reader.string();
            auto program = readProgram(reader);
            if (program.p9.empty())
                throw ProjectStateError("browser program is missing its P9 data");
            program.fileName = fileName;
            result.browserPrograms.push_back(std::move(program));
        }
        if (result.browserPrograms.empty() && result.selectedProgramIndex != 0)
            throw ProjectStateError("selected program requires a browser collection");
        if (!result.browserPrograms.empty()) {
            if (result.selectedProgramIndex >= result.browserPrograms.size())
                throw ProjectStateError("selected program index is out of range");
            const auto& selected = result.browserPrograms[result.selectedProgramIndex];
            if (result.p9 != selected.p9 || result.s9Samples != selected.s9Samples)
                throw ProjectStateError(
                    "active program does not match the selected browser program");
        }
    }
    if (version == pitchBendFormatVersion || version == formatVersion) {
        result.pitchBendRangeSemitones = reader.u32();
        if (result.pitchBendRangeSemitones < 1 || result.pitchBendRangeSemitones > 12)
            throw ProjectStateError("pitch-bend range is outside 1-12 semitones");
    }
    if (version == formatVersion) {
        const auto omni = reader.u32();
        if (omni > 1)
            throw ProjectStateError("invalid MIDI Omni setting");
        result.midiOmni = omni != 0;
        result.basicMidiChannel = reader.u32();
        if (result.basicMidiChannel < 1 || result.basicMidiChannel > 16)
            throw ProjectStateError("basic MIDI channel is outside 1-16");
    }
    if (reader.remaining() != 0)
        throw ProjectStateError("unexpected trailing project-state data");
    return result;
}

} // namespace e45recordings::play950::state
