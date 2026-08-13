#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace e45recordings::play950::state {

struct EmbeddedProgram {
    std::string fileName;
    std::vector<std::byte> p9;
    std::vector<std::vector<std::byte>> s9Samples;

    bool operator==(const EmbeddedProgram&) const = default;
};

struct ProjectState {
    std::vector<std::byte> p9;
    std::vector<std::vector<std::byte>> s9Samples;
    std::string sourceName;
    std::string sourcePath;
    std::vector<EmbeddedProgram> browserPrograms;
    std::uint32_t selectedProgramIndex {0};
    std::uint32_t pitchBendRangeSemitones {2};
    bool midiOmni {true};
    std::uint32_t basicMidiChannel {1};

    [[nodiscard]] bool hasProgram() const noexcept { return !p9.empty(); }
    bool operator==(const ProjectState&) const = default;
};

class ProjectStateError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] std::vector<std::byte> serializeProjectState(const ProjectState& state);
[[nodiscard]] ProjectState deserializeProjectState(std::span<const std::byte> bytes);

} // namespace e45recordings::play950::state
