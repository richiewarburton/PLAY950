#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace e45recordings::play950::workflow {

[[nodiscard]] std::vector<std::string> rememberRecentImage(
    std::span<const std::string> existing, const std::string& successfulPath,
    std::size_t maximumCount = 8);
[[nodiscard]] std::vector<std::string> removeMissingImages(
    std::span<const std::string> existing);
[[nodiscard]] std::size_t selectionAfterReload(
    std::span<const std::string> newFileNames, const std::string& previousFileName) noexcept;

} // namespace e45recordings::play950::workflow
