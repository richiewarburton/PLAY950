#include "workflow/ImageWorkflow.h"

#include <algorithm>
#include <filesystem>

namespace e45recordings::play950::workflow {

std::vector<std::string> rememberRecentImage(
    std::span<const std::string> existing, const std::string& successfulPath,
    std::size_t maximumCount) {
    std::vector<std::string> result;
    if (!successfulPath.empty() && maximumCount > 0)
        result.push_back(successfulPath);
    for (const auto& path : existing) {
        if (path.empty() || result.size() >= maximumCount ||
            std::find(result.begin(), result.end(), path) != result.end())
            continue;
        result.push_back(path);
    }
    return result;
}

std::vector<std::string> removeMissingImages(std::span<const std::string> existing) {
    std::vector<std::string> result;
    std::error_code error;
    for (const auto& path : existing) {
        error.clear();
        if (!path.empty() && std::filesystem::is_regular_file(path, error) && !error)
            result.push_back(path);
    }
    return result;
}

std::size_t selectionAfterReload(
    std::span<const std::string> newFileNames, const std::string& previousFileName) noexcept {
    if (previousFileName.empty())
        return 0;
    const auto found = std::find(newFileNames.begin(), newFileNames.end(), previousFileName);
    return found == newFileNames.end()
        ? 0 : static_cast<std::size_t>(found - newFileNames.begin());
}

} // namespace e45recordings::play950::workflow
