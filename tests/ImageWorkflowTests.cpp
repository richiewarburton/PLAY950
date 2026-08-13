#include "workflow/ImageWorkflow.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace e45recordings::play950::workflow;

void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

int main() {
    try {
        const std::vector<std::string> existing {"/a/ONE.img", "/b/TWO.img", "/a/ONE.img"};
        const auto remembered = rememberRecentImage(existing, "/b/TWO.img", 3);
        require(remembered == std::vector<std::string>({"/b/TWO.img", "/a/ONE.img"}),
                "successful recent image was not promoted and deduplicated");

        const std::vector<std::string> names {"ADG.P9", "ADGPASTE.P9", "ADGPASTE2.P9"};
        require(selectionAfterReload(names, "ADGPASTE2.P9") == 2,
                "reload did not preserve a matching P9 filename");
        require(selectionAfterReload(names, "REMOVED.P9") == 0,
                "reload did not fall back after program removal");

        const auto present = std::filesystem::temp_directory_path() /
                             "play950-workflow-present.img";
        { std::ofstream stream(present); stream << "fixture"; }
        const std::vector<std::string> paths {present.string(),
                                             (present.string() + ".missing")};
        require(removeMissingImages(paths) == std::vector<std::string>({present.string()}),
                "missing recent images were not removed");
        std::error_code ignored;
        std::filesystem::remove(present, ignored);

        std::cout << "PLAY950 image-workflow tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PLAY950 image-workflow tests failed: " << error.what() << '\n';
        return 1;
    }
}
