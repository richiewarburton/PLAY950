#include "content/ContentLoader.h"

#include "formats/P9.h"
#include "formats/S9.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace e45recordings::play950::content {
namespace {

std::vector<std::byte> readBinaryFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        throw ContentLoadError("could not open " + path.filename().string());
    const auto fileSize = stream.tellg();
    if (fileSize <= 0)
        throw ContentLoadError(path.filename().string() + " is empty");
    stream.seekg(0);
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    stream.read(reinterpret_cast<char*>(bytes.data()), fileSize);
    if (!stream)
        throw ContentLoadError("could not read " + path.filename().string());
    return bytes;
}

bool hasExtension(const std::filesystem::path& path, const std::string& wanted) {
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return extension == wanted;
}

std::vector<std::filesystem::path> filesWithExtension(
    const std::filesystem::path& directory, const std::string& extension) {
    std::vector<std::filesystem::path> result;
    std::error_code error;
    for (std::filesystem::directory_iterator iterator(directory, error), end;
         !error && iterator != end; iterator.increment(error)) {
        if (iterator->is_regular_file(error) && hasExtension(iterator->path(), extension))
            result.push_back(iterator->path());
    }
    if (error)
        throw ContentLoadError("could not enumerate " + directory.filename().string());
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace

state::ProjectState loadP9WithLinkedSamples(const std::filesystem::path& p9Path) {
    try {
        state::ProjectState result;
        result.p9 = readBinaryFile(p9Path);
        const auto program = formats::parseP9(result.p9);

        std::set<std::string> requiredNames;
        for (const auto& keygroup : program.keygroups) {
            if (!keygroup.softSampleName.empty() &&
                !formats::isDefaultSamplePlaceholder(keygroup.softSampleName))
                requiredNames.insert(keygroup.softSampleName);
            if (!keygroup.loudSampleName.empty() &&
                !formats::isDefaultSamplePlaceholder(keygroup.loudSampleName))
                requiredNames.insert(keygroup.loudSampleName);
        }

        std::vector<formats::S9Sample> parsedSamples;
        for (const auto& candidate : filesWithExtension(p9Path.parent_path(), ".s9")) {
            auto bytes = readBinaryFile(candidate);
            auto parsed = formats::parseUncompressedS9(bytes);
            if (!requiredNames.contains(parsed.name))
                continue;
            const bool duplicate = std::any_of(
                parsedSamples.begin(), parsedSamples.end(), [&](const formats::S9Sample& sample) {
                    return sample.name == parsed.name;
                });
            if (!duplicate) {
                parsedSamples.push_back(std::move(parsed));
                result.s9Samples.push_back(std::move(bytes));
            }
        }

        return result;
    } catch (const ContentLoadError&) {
        throw;
    } catch (const std::exception& error) {
        throw ContentLoadError(error.what());
    }
}

LoadedProgram loadP9Program(const std::filesystem::path& p9Path) {
    try {
        auto projectState = loadP9WithLinkedSamples(p9Path);
        const auto parsed = formats::parseP9(projectState.p9);
        return {parsed.name.empty() ? p9Path.stem().string() : parsed.name,
                p9Path.filename().string(), std::move(projectState)};
    } catch (const ContentLoadError& error) {
        throw ContentLoadError(p9Path.filename().string() + ": " + error.what());
    }
}

std::vector<LoadedProgram> loadP9ProgramsInDirectory(
    const std::filesystem::path& directory) {
    std::vector<LoadedProgram> programs;
    for (const auto& path : filesWithExtension(directory, ".p9"))
        programs.push_back(loadP9Program(path));
    if (programs.empty())
        throw ContentLoadError("the image contains no P9 program");
    return programs;
}

std::filesystem::path firstP9InDirectory(const std::filesystem::path& directory) {
    const auto programs = filesWithExtension(directory, ".p9");
    if (programs.empty())
        throw ContentLoadError("the image contains no P9 program");
    return programs.front();
}

} // namespace e45recordings::play950::content
