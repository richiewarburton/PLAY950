#include "audio/SampleVoicePool.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using e45recordings::play950::audio::PreparedSample;
using e45recordings::play950::audio::SampleVoicePool;
using e45recordings::play950::formats::S9PlaybackMode;
using e45recordings::play950::formats::S9Direction;

void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

int main() {
    try {
        e45recordings::play950::formats::S9Sample parsedReverse;
        parsedReverse.samples12 = {0, 1};
        parsedReverse.sampleRate = 48'000;
        parsedReverse.nominalPitchSixteenths = 60 * 16;
        parsedReverse.playbackEnd = 2;
        parsedReverse.direction = S9Direction::reverse;
        parsedReverse.loudnessOffset = 5;
        const auto preparedReverse =
            e45recordings::play950::audio::prepareSample(std::move(parsedReverse));
        require(preparedReverse.direction == S9Direction::reverse &&
                    preparedReverse.loudnessOffset == 5,
                "sample preparation discarded native playback metadata");

        SampleVoicePool pool;
        PreparedSample sample;
        sample.samples12 = {0, 1024, 2047, 1024, 0, -1024, -2048, -1024};
        sample.sampleRate = 48'000;
        sample.nominalPitchSixteenths = 60 * 16;
        sample.playbackStart = 0;
        sample.playbackEnd = sample.samples12.size();
        sample.loopStart = 0;
        sample.playbackMode = S9PlaybackMode::loop;
        pool.setSample(std::move(sample));
        pool.setHostSampleRate(48'000.0);

        for (int i = 0; i < 8; ++i)
            pool.noteOn(60 + i, 100 + i, 0.5F);
        require(pool.activeVoiceCount() == 8, "pool did not allocate eight voices");

        pool.noteOn(72, 200, 0.5F);
        require(pool.activeVoiceCount() == 8, "ninth note changed voice count");
        require(!pool.isNoteActive(60, 100), "ninth note did not steal the oldest voice");
        require(pool.isNoteActive(72, 200), "stolen slot did not start the new note");

        pool.noteOff(64, 104);
        require(!pool.isNoteActive(64, 104) && pool.activeVoiceCount() == 7,
                "note-off did not release its note-id voice");

        std::array<float, 64> output {};
        pool.renderAdd(output.data(), output.size());
        require(std::any_of(output.begin(), output.end(), [](float value) {
                    return std::abs(value) > 0.0001F;
                }),
                "voice pool rendered silence");
        require(pool.activeVoiceCount() == 7, "looping voices stopped during render");

        SampleVoicePool bendPool;
        PreparedSample bendSample;
        bendSample.samples12 = {0, 256, 512, 768, 1024, 1280, 1536, 1792};
        bendSample.sampleRate = 48'000;
        bendSample.nominalPitchSixteenths = 60 * 16;
        bendSample.playbackEnd = bendSample.samples12.size();
        bendSample.playbackMode = S9PlaybackMode::loop;
        bendPool.setSample(std::move(bendSample));
        bendPool.setHostSampleRate(48'000.0);
        bendPool.noteOn(60, 250, 1.0F);
        bendPool.setPitchBend(1.0F, 12);
        std::array<float, 2> bentOutput {};
        bendPool.renderAdd(bentOutput.data(), bentOutput.size());
        require(std::abs(bentOutput[1] - 0.25F) < 0.0001F,
                "full-up 12-semitone pitch bend did not double playback rate");

        pool.noteOff(0, -1);
        require(pool.activeVoiceCount() == 7, "unmatched pitch note-off changed voices");

        SampleVoicePool reversePool;
        PreparedSample reverse;
        reverse.samples12 = {0, 256, 512, 768};
        reverse.sampleRate = 48'000;
        reverse.nominalPitchSixteenths = 60 * 16;
        reverse.playbackEnd = reverse.samples12.size();
        reverse.playbackMode = S9PlaybackMode::oneShot;
        reverse.direction = S9Direction::reverse;
        reversePool.setSample(std::move(reverse));
        reversePool.setHostSampleRate(48'000.0);
        reversePool.noteOn(60, 300, 1.0F);
        std::array<float, 5> reverseOutput {};
        reversePool.renderAdd(reverseOutput.data(), reverseOutput.size());
        require(std::abs(reverseOutput[0] - 0.375F) < 0.0001F &&
                    std::abs(reverseOutput[1] - 0.25F) < 0.0001F &&
                    std::abs(reverseOutput[2] - 0.125F) < 0.0001F &&
                    std::abs(reverseOutput[3]) < 0.0001F &&
                    reversePool.activeVoiceCount() == 0,
                "reverse one-shot traversal failed");

        SampleVoicePool alternatingPool;
        PreparedSample alternating;
        alternating.samples12 = {0, 256, 512, 768};
        alternating.sampleRate = 48'000;
        alternating.nominalPitchSixteenths = 60 * 16;
        alternating.playbackEnd = alternating.samples12.size();
        alternating.loopStart = 1;
        alternating.playbackMode = S9PlaybackMode::alternatingLoop;
        alternatingPool.setSample(std::move(alternating));
        alternatingPool.setHostSampleRate(48'000.0);
        alternatingPool.noteOn(60, 301, 1.0F);
        std::array<float, 8> alternatingOutput {};
        alternatingPool.renderAdd(alternatingOutput.data(), alternatingOutput.size());
        const std::array<float, 8> expectedAlternating {
            0.0F, 0.125F, 0.25F, 0.375F, 0.25F, 0.125F, 0.25F, 0.375F};
        for (std::size_t index = 0; index < alternatingOutput.size(); ++index)
            require(std::abs(alternatingOutput[index] - expectedAlternating[index]) < 0.0001F,
                    "alternating-loop traversal failed");
        require(alternatingPool.activeVoiceCount() == 1,
                "alternating-loop voice stopped unexpectedly");

        std::cout << "PLAY950 voice-pool tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PLAY950 voice-pool tests failed: " << error.what() << '\n';
        return 1;
    }
}
