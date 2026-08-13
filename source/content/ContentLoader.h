#pragma once

#include "state/ProjectState.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace e45recordings::play950::content {

class ContentLoadError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct LoadedProgram {
    std::string nativeName;
    std::string fileName;
    state::ProjectState state;
};

[[nodiscard]] state::ProjectState loadP9WithLinkedSamples(
    const std::filesystem::path& p9Path);
[[nodiscard]] LoadedProgram loadP9Program(const std::filesystem::path& p9Path);
[[nodiscard]] std::vector<LoadedProgram> loadP9ProgramsInDirectory(
    const std::filesystem::path& directory);
[[nodiscard]] std::filesystem::path firstP9InDirectory(
    const std::filesystem::path& directory);

} // namespace e45recordings::play950::content
