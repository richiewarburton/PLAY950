#include "content/ContentLoader.h"
#include "audio/SampleVoicePool.h"
#include "formats/P9.h"
#include "formats/S9.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

using namespace e45recordings::play950;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

class TemporaryDirectory {
public:
    explicit TemporaryDirectory(const std::filesystem::path& fixtureRoot) {
        path_ = std::filesystem::temp_directory_path() / "play950-content-loader-tests";
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
        std::filesystem::create_directory(path_);
        for (const auto& entry : std::filesystem::directory_iterator(fixtureRoot)) {
            if (entry.path().extension() == ".S9")
                std::filesystem::copy_file(entry.path(), path_ / entry.path().filename());
        }
        std::filesystem::copy_file(fixtureRoot / "ALWAYS.P9", path_ / "B.P9");
        std::filesystem::copy_file(fixtureRoot / "ALWAYS.P9", path_ / "A.P9");
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 4 && std::string(argv[1]) == "--program-directory") {
            const auto programs = content::loadP9ProgramsInDirectory(argv[2]);
            require(programs.size() == static_cast<std::size_t>(std::stoul(argv[3])),
                    "genuine multi-program count failed");
            for (const auto& program : programs) {
                require(!program.nativeName.empty(), "empty genuine program name");
                require(!program.state.p9.empty(), "empty genuine P9 state");
                for (const auto& sampleBytes : program.state.s9Samples) {
                    auto parsedSample = formats::parseUncompressedS9(sampleBytes);
                    const auto direction = parsedSample.direction;
                    const auto preparedSample = audio::prepareSample(std::move(parsedSample));
                    require(preparedSample.direction == direction,
                            "genuine sample direction was lost during preparation");
                }
            }
            std::cout << "PLAY950 genuine multi-program loader passed:";
            for (const auto& program : programs)
                std::cout << ' ' << program.nativeName;
            std::cout << '\n';
            return 0;
        }
        require(argc == 2, "fixture directory argument is required");
        const auto root = std::filesystem::path(argv[1]);
        const auto loaded = content::loadP9WithLinkedSamples(root / "ALWAYS.P9");
        require(loaded.s9Samples.size() == 14, "wrong linked sample count");

        const auto program = formats::parseP9(loaded.p9);
        require(program.name == "ALWAYS", "wrong program selected");
        for (const auto& bytes : loaded.s9Samples)
            (void)formats::parseUncompressedS9(bytes);

        require(content::firstP9InDirectory(root).filename() == "ALWAYS.P9",
                "directory program selection failed");

        const auto single = content::loadP9ProgramsInDirectory(root);
        require(single.size() == 1 && single.front().nativeName == "ALWAYS" &&
                    single.front().fileName == "ALWAYS.P9",
                "single-program discovery failed");

        TemporaryDirectory multiple(root);
        const auto programs = content::loadP9ProgramsInDirectory(multiple.path());
        require(programs.size() == 2, "multi-program discovery count failed");
        require(programs[0].fileName == "A.P9" && programs[1].fileName == "B.P9",
                "multi-program filename ordering failed");
        require(programs[0].nativeName == "ALWAYS" && programs[1].nativeName == "ALWAYS",
                "multi-program native-name decoding failed");
        require(programs[0].state.s9Samples.size() == 14 &&
                    programs[1].state.s9Samples.size() == 14,
                "multi-program linked content failed");

        std::filesystem::remove(multiple.path() / "ALWAYS01.S9");
        const auto incomplete = content::loadP9Program(multiple.path() / "A.P9");
        require(incomplete.state.s9Samples.size() == 13,
                "program with a missing linked sample did not load its remaining samples");
        std::cout << "PLAY950 content-loader tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PLAY950 content-loader tests failed: " << error.what() << '\n';
        return 1;
    }
}
