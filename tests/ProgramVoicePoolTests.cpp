#include "audio/ProgramVoicePool.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace e45recordings::play950;

void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

audio::PreparedSample loopingRamp() {
    audio::PreparedSample sample;
    sample.samples12 = {0, 256, 512, 768, 1024, 1280, 1536, 1792};
    sample.sampleRate = 48'000;
    sample.nominalPitchSixteenths = 60 * 16;
    sample.playbackStart = 0;
    sample.playbackEnd = sample.samples12.size();
    sample.loopStart = 0;
    sample.playbackMode = formats::S9PlaybackMode::loop;
    return sample;
}

audio::PreparedKeygroup keygroup(int note, std::uint8_t output, std::int16_t tuning = 0) {
    return {static_cast<std::uint8_t>(note), static_cast<std::uint8_t>(note), 0,
            std::nullopt, 128, tuning, 0, formats::P9Output {output}};
}

int main() {
    try {
        audio::ProgramVoicePool pool;
        audio::PreparedProgram program;
        program.samples.push_back(loopingRamp());
        program.keygroups.push_back(keygroup(60, 0, 192));
        program.keygroups.push_back(keygroup(61, 1));
        program.keygroups.push_back(keygroup(62, 2));
        program.keygroups.push_back(keygroup(63, 3));
        program.keygroups.push_back(keygroup(64, 8));
        program.keygroups.push_back(keygroup(65, 4));
        pool.setProgram(std::move(program));
        pool.setHostSampleRate(48'000.0);

        pool.noteOn(59, 99, 1.0F);
        require(pool.activeVoiceCount() == 0, "out-of-range note selected a keygroup");
        pool.noteOn(60, 100, 1.0F);

        std::array<std::array<float, 2>, audio::ProgramVoicePool::outputCount> storage {};
        audio::ProgramVoicePool::OutputBuffers outputs {};
        for (std::size_t bus = 0; bus < outputs.size(); ++bus)
            outputs[bus] = storage[bus].data();
        pool.renderAdd(outputs, 2);
        require(std::abs(storage[0][1] - 0.25F) < 0.0001F,
                "P9 sixteenth-semitone tuning was not applied");
        require(storage[1][1] == storage[0][1], "Mono(01) did not receive its voice");
        require(storage[9][1] == storage[0][1], "Left(09) did not aggregate Mono(01)");
        require(storage[10][1] == 0.0F, "Mono(01) leaked to Right(10)");

        audio::ProgramVoicePool bendPool;
        audio::PreparedProgram bendProgram;
        bendProgram.samples.push_back(loopingRamp());
        bendProgram.keygroups.push_back(keygroup(60, 0));
        bendPool.setProgram(std::move(bendProgram));
        bendPool.setHostSampleRate(48'000.0);
        bendPool.noteOn(60, 99, 1.0F);
        bendPool.setPitchBend(1.0F, 12);
        storage = {};
        bendPool.renderAdd(outputs, 2);
        require(std::abs(storage[0][1] - 0.25F) < 0.0001F,
                "program voice did not apply full-up 12-semitone pitch bend");

        audio::ProgramVoicePool channelPool;
        audio::PreparedProgram channelProgram;
        channelProgram.samples.push_back(loopingRamp());
        auto quieterRamp = loopingRamp();
        for (auto& value : quieterRamp.samples12)
            value = static_cast<std::int16_t>(value / 2);
        channelProgram.samples.push_back(std::move(quieterRamp));
        auto channelOne = keygroup(60, 0);
        channelOne.output.raw = 0xff;
        auto channelTwo = channelOne;
        channelTwo.softSampleIndex = 1;
        channelTwo.midiChannelOffset = 1;
        channelProgram.keygroups.push_back(channelOne);
        channelProgram.keygroups.push_back(channelTwo);
        channelPool.setProgram(std::move(channelProgram));
        channelPool.setHostSampleRate(48'000.0);
        channelPool.setMidiReception(false, 1);

        channelPool.noteOn(1, 60, 300, 1.0F);
        storage = {};
        channelPool.renderAdd(outputs, 2);
        require(std::abs(storage[0][1] - 0.0625F) < 0.0001F,
                "MIDI channel 2 did not select its overlapping keygroup");
        channelPool.noteOff(1, 60, 300);
        channelPool.noteOn(2, 60, 301, 1.0F);
        require(channelPool.activeVoiceCount() == 0,
                "unassigned MIDI channel selected a keygroup");

        audio::ProgramVoicePool isolatedBendPool;
        audio::PreparedProgram isolatedBendProgram;
        isolatedBendProgram.samples.push_back(loopingRamp());
        auto bendChannelOne = keygroup(60, 0);
        auto bendChannelTwo = keygroup(60, 4);
        bendChannelTwo.midiChannelOffset = 1;
        isolatedBendProgram.keygroups.push_back(bendChannelOne);
        isolatedBendProgram.keygroups.push_back(bendChannelTwo);
        isolatedBendPool.setProgram(std::move(isolatedBendProgram));
        isolatedBendPool.setHostSampleRate(48'000.0);
        isolatedBendPool.setMidiReception(false, 1);
        isolatedBendPool.noteOn(0, 60, -1, 1.0F);
        isolatedBendPool.noteOn(1, 60, -1, 1.0F);
        isolatedBendPool.setPitchBend(0, 1.0F, 12);
        storage = {};
        isolatedBendPool.renderAdd(outputs, 2);
        require(std::abs(storage[1][1] - 0.25F) < 0.0001F &&
                    std::abs(storage[5][1] - 0.125F) < 0.0001F,
                "pitch bend leaked between MIDI channels");
        isolatedBendPool.noteOff(0, 60, -1);
        require(!isolatedBendPool.isNoteActive(0, 60, -1) &&
                    isolatedBendPool.isNoteActive(1, 60, -1),
                "note-off leaked between MIDI channels");
        isolatedBendPool.noteOff(1, 60, -1);

        isolatedBendPool.setMidiReception(false, 16);
        isolatedBendPool.noteOn(0, 60, 302, 1.0F);
        require(isolatedBendPool.isNoteActive(0, 60, 302),
                "wrapped basic-channel offset did not select MIDI channel 1");
        isolatedBendPool.noteOff(0, 60, 302);

        pool.noteOn(60, 101, 1.0F);
        require(pool.activeVoiceCount() == 1, "mono output accepted two voices");
        require(pool.isNoteActive(60, 100) && !pool.isNoteActive(60, 101),
                "mono output replacement was not deferred");
        storage = {};
        pool.renderAdd(outputs, 2);
        storage = {};
        pool.renderAdd(outputs, 2);
        require(!pool.isNoteActive(60, 100) && pool.isNoteActive(60, 101),
                "mono output did not complete its deferred replacement");

        pool.noteOn(61, 102, 1.0F);
        pool.noteOn(62, 103, 1.0F);
        pool.noteOn(63, 104, 1.0F);
        require(pool.activeVoiceCount() == 4, "Left group did not accept four voices");
        pool.noteOn(64, 105, 1.0F);
        require(pool.activeVoiceCount() == 4 && !pool.isNoteActive(60, 101),
                "fifth Left voice did not steal the oldest Left voice");

        pool.noteOn(65, 106, 1.0F);
        require(pool.activeVoiceCount() == 5, "Right group incorrectly reduced Left polyphony");

        auto handoverProgram = [] {
            audio::PreparedProgram result;
            auto oldSample = loopingRamp();
            oldSample.samples12 = {1024, 512, -512, -1024};
            oldSample.nominalPitchSixteenths = 80 * 16;
            oldSample.playbackEnd = oldSample.samples12.size();
            result.samples.push_back(oldSample);
            auto replacement = oldSample;
            replacement.samples12 = {1536, 1024, 512, 256};
            replacement.nominalPitchSixteenths = 81 * 16;
            result.samples.push_back(replacement);
            auto newest = oldSample;
            newest.samples12 = {256, 256, 256, 256};
            newest.nominalPitchSixteenths = 82 * 16;
            result.samples.push_back(newest);
            auto oldKeygroup = keygroup(80, 0);
            auto replacementKeygroup = keygroup(81, 0);
            replacementKeygroup.softSampleIndex = 1;
            auto newestKeygroup = keygroup(82, 0);
            newestKeygroup.softSampleIndex = 2;
            result.keygroups = {oldKeygroup, replacementKeygroup, newestKeygroup};
            return result;
        };

        audio::ProgramVoicePool freshReplacementPool;
        freshReplacementPool.setProgram(handoverProgram());
        freshReplacementPool.setHostSampleRate(48'000.0);
        freshReplacementPool.noteOn(81, 401, 1.0F);
        std::array<std::array<float, 4>, audio::ProgramVoicePool::outputCount>
            freshStorage {};
        audio::ProgramVoicePool::OutputBuffers freshOutputs {};
        for (std::size_t bus = 0; bus < freshOutputs.size(); ++bus)
            freshOutputs[bus] = freshStorage[bus].data();
        freshReplacementPool.renderAdd(freshOutputs, 2);

        audio::ProgramVoicePool handoverPool;
        handoverPool.setProgram(handoverProgram());
        handoverPool.setHostSampleRate(48'000.0);
        handoverPool.noteOn(80, 400, 1.0F);
        std::array<std::array<float, 4>, audio::ProgramVoicePool::outputCount>
            handoverStorage {};
        audio::ProgramVoicePool::OutputBuffers handoverOutputs {};
        for (std::size_t bus = 0; bus < handoverOutputs.size(); ++bus)
            handoverOutputs[bus] = handoverStorage[bus].data();
        handoverPool.renderAdd(handoverOutputs, 1);
        handoverPool.noteOn(81, 401, 1.0F);
        require(handoverPool.isNoteActive(80, 400) &&
                    !handoverPool.isNoteActive(81, 401),
                "individual-output replacement did not remain pending");
        handoverStorage = {};
        handoverPool.renderAdd(handoverOutputs, 4);
        require(std::abs(handoverStorage[0][0] - 0.25F) < 0.0001F &&
                    std::abs(handoverStorage[0][1] + 0.25F) < 0.0001F,
                "retiring voice did not render through its sign-changing zero crossing");
        require(handoverStorage[0][0] > 0.0F && handoverStorage[0][1] < 0.0F,
                "waveform did not bracket zero at the handover crossing");
        require(std::abs(handoverStorage[0][2] - freshStorage[0][0]) < 0.0001F &&
                    std::abs(handoverStorage[0][3] - freshStorage[0][1]) < 0.0001F,
                "replacement transient, playback start, or envelopes were modified");
        require(!handoverPool.isNoteActive(80, 400) &&
                    handoverPool.isNoteActive(81, 401) &&
                    handoverPool.activeVoiceCount() == 1,
                "old and replacement voices overlapped after handover");

        audio::ProgramVoicePool priorityPool;
        priorityPool.setProgram(handoverProgram());
        priorityPool.setHostSampleRate(48'000.0);
        priorityPool.noteOn(80, 410, 1.0F);
        freshStorage = {};
        priorityPool.renderAdd(freshOutputs, 1);
        priorityPool.noteOn(81, 411, 1.0F);
        priorityPool.noteOn(82, 412, 1.0F);
        freshStorage = {};
        priorityPool.renderAdd(freshOutputs, 4);
        require(std::abs(freshStorage[0][2] - 0.125F) < 0.0001F &&
                    priorityPool.isNoteActive(82, 412) &&
                    !priorityPool.isNoteActive(81, 411),
                "newest pending note did not win monophonic handover");

        audio::ProgramVoicePool cancelledPool;
        cancelledPool.setProgram(handoverProgram());
        cancelledPool.setHostSampleRate(48'000.0);
        cancelledPool.noteOn(80, 420, 1.0F);
        freshStorage = {};
        cancelledPool.renderAdd(freshOutputs, 1);
        cancelledPool.noteOn(81, 421, 1.0F);
        cancelledPool.noteOff(81, 421);
        freshStorage = {};
        cancelledPool.renderAdd(freshOutputs, 4);
        require(cancelledPool.isNoteActive(80, 420) &&
                    !cancelledPool.isNoteActive(81, 421),
                "note-off did not cancel a pending non-One-Shot note");

        auto oneShotProgram = handoverProgram();
        oneShotProgram.keygroups[1].oneShot = true;
        audio::ProgramVoicePool oneShotPendingPool;
        oneShotPendingPool.setProgram(std::move(oneShotProgram));
        oneShotPendingPool.setHostSampleRate(48'000.0);
        oneShotPendingPool.noteOn(80, 430, 1.0F);
        freshStorage = {};
        oneShotPendingPool.renderAdd(freshOutputs, 1);
        oneShotPendingPool.noteOn(81, 431, 1.0F);
        oneShotPendingPool.noteOff(81, 431);
        freshStorage = {};
        oneShotPendingPool.renderAdd(freshOutputs, 4);
        require(oneShotPendingPool.isNoteActive(81, 431),
                "note-off incorrectly cancelled a pending One Shot note");

        auto timeoutProgram = handoverProgram();
        timeoutProgram.samples[0].samples12 = {1024, 1024};
        timeoutProgram.samples[0].playbackEnd = 2;
        audio::ProgramVoicePool timeoutPool;
        timeoutPool.setProgram(std::move(timeoutProgram));
        timeoutPool.setHostSampleRate(1'000.0);
        timeoutPool.noteOn(80, 440, 1.0F);
        freshStorage = {};
        timeoutPool.renderAdd(freshOutputs, 1);
        timeoutPool.noteOn(81, 441, 1.0F);
        freshStorage = {};
        timeoutPool.renderAdd(freshOutputs, 2);
        require(std::abs(freshStorage[0][0] - 0.5F) < 0.0001F &&
                    std::abs(freshStorage[0][1] - 0.75F) < 0.0001F &&
                    timeoutPool.isNoteActive(81, 441),
                "1.0 ms monophonic handover timeout was not enforced");

        audio::ProgramVoicePool layerPool;
        audio::PreparedProgram layerProgram;
        layerProgram.samples.push_back(loopingRamp());
        auto layered = keygroup(66, 5);
        layered.loudSampleIndex = 1;
        layered.velocityThreshold = 64;
        layered.softTuningSixteenths = -96;
        layered.loudTuningSixteenths = 96;
        auto loud = loopingRamp();
        for (auto& value : loud.samples12)
            value = static_cast<std::int16_t>(value / 2);
        layerProgram.samples.push_back(std::move(loud));
        layerProgram.keygroups.push_back(layered);
        auto missingLoud = keygroup(67, 5);
        missingLoud.velocityThreshold = 64;
        layerProgram.keygroups.push_back(missingLoud);
        layerPool.setProgram(std::move(layerProgram));
        layerPool.setHostSampleRate(48'000.0);

        layerPool.noteOn(66, 107, 0.25F);
        storage = {};
        layerPool.renderAdd(outputs, 2);
        require(std::abs(storage[0][1] - 0.125F) < 0.0001F,
                "Soft layer selection or velocity-independent gain failed");
        layerPool.noteOff(66, 107);
        layerPool.noteOn(66, 108, 1.0F);
        storage = {};
        layerPool.renderAdd(outputs, 2);
        require(std::abs(storage[0][1] - 0.125F) < 0.0001F,
                "Loud layer selection or layer-specific tuning failed");
        layerPool.noteOff(66, 108);
        layerPool.noteOn(67, 109, 1.0F);
        require(layerPool.activeVoiceCount() == 0,
                "missing selected Loud layer did not remain silent");

        audio::ProgramVoicePool flagPool;
        audio::PreparedProgram flagProgram;
        flagProgram.samples.push_back(loopingRamp());
        auto oneShot = keygroup(72, 6);
        oneShot.oneShot = true;
        flagProgram.keygroups.push_back(oneShot);
        auto constantPitch = keygroup(73, 7);
        constantPitch.constantPitch = true;
        flagProgram.keygroups.push_back(constantPitch);
        flagPool.setProgram(std::move(flagProgram));
        flagPool.setHostSampleRate(48'000.0);

        flagPool.noteOn(73, 111, 1.0F);
        storage = {};
        flagPool.renderAdd(outputs, 2);
        require(std::abs(storage[0][1] - 0.125F) < 0.0001F,
                "P9 Constant Pitch followed the played MIDI note");
        flagPool.noteOff(73, 111);

        flagPool.noteOn(72, 110, 1.0F);
        flagPool.noteOff(72, 110);
        require(flagPool.isNoteActive(72, 110),
                "P9 One Shot voice stopped on note-off");

        audio::ProgramVoicePool directionPool;
        audio::PreparedProgram directionProgram;
        auto reverse = loopingRamp();
        reverse.samples12 = {0, 256, 512, 768};
        reverse.nominalPitchSixteenths = 74 * 16;
        reverse.playbackEnd = reverse.samples12.size();
        reverse.playbackMode = formats::S9PlaybackMode::oneShot;
        reverse.direction = formats::S9Direction::reverse;
        directionProgram.samples.push_back(reverse);
        auto alternating = reverse;
        alternating.nominalPitchSixteenths = 75 * 16;
        alternating.direction = formats::S9Direction::forward;
        alternating.playbackMode = formats::S9PlaybackMode::alternatingLoop;
        alternating.loopStart = 1;
        directionProgram.samples.push_back(alternating);
        auto reverseKeygroup = keygroup(74, 7);
        directionProgram.keygroups.push_back(reverseKeygroup);
        auto alternatingKeygroup = keygroup(75, 7);
        alternatingKeygroup.softSampleIndex = 1;
        directionProgram.keygroups.push_back(alternatingKeygroup);
        directionPool.setProgram(std::move(directionProgram));
        directionPool.setHostSampleRate(48'000.0);

        directionPool.noteOn(74, 112, 1.0F);
        std::array<std::array<float, 8>, audio::ProgramVoicePool::outputCount>
            directionStorage {};
        audio::ProgramVoicePool::OutputBuffers directionOutputs {};
        for (std::size_t bus = 0; bus < directionOutputs.size(); ++bus)
            directionOutputs[bus] = directionStorage[bus].data();
        directionPool.renderAdd(directionOutputs, 5);
        require(std::abs(directionStorage[0][0] - 0.375F) < 0.0001F &&
                    std::abs(directionStorage[0][1] - 0.25F) < 0.0001F &&
                    std::abs(directionStorage[0][2] - 0.125F) < 0.0001F &&
                    std::abs(directionStorage[0][3]) < 0.0001F &&
                    directionPool.activeVoiceCount() == 0,
                "program reverse one-shot traversal failed");

        directionPool.noteOn(75, 113, 1.0F);
        directionStorage = {};
        directionPool.renderAdd(directionOutputs, 8);
        const std::array<float, 8> expectedProgramAlternating {
            0.0F, 0.125F, 0.25F, 0.375F, 0.25F, 0.125F, 0.25F, 0.375F};
        for (std::size_t index = 0; index < expectedProgramAlternating.size(); ++index)
            require(std::abs(directionStorage[0][index] - expectedProgramAlternating[index]) <
                        0.0001F,
                    "program alternating-loop traversal failed");

        auto filteredRms = [](audio::PreparedKeygroup testKeygroup, float velocity,
                              int pitch = 60, int sampleLoudness = 0) {
            audio::PreparedProgram testProgram;
            auto signal = loopingRamp();
            signal.samples12 = {1024, -1024};
            signal.playbackEnd = signal.samples12.size();
            signal.loudnessOffset = static_cast<std::int16_t>(sampleLoudness);
            testProgram.samples.push_back(std::move(signal));
            testKeygroup.lowKey = static_cast<std::uint8_t>(pitch);
            testKeygroup.highKey = static_cast<std::uint8_t>(pitch);
            testProgram.keygroups.push_back(testKeygroup);
            audio::ProgramVoicePool testPool;
            testPool.setProgram(std::move(testProgram));
            testPool.setHostSampleRate(48'000.0);
            testPool.noteOn(pitch, 200, velocity);
            std::array<std::array<float, 4096>, audio::ProgramVoicePool::outputCount>
                testStorage {};
            audio::ProgramVoicePool::OutputBuffers testOutputs {};
            for (std::size_t bus = 0; bus < testOutputs.size(); ++bus)
                testOutputs[bus] = testStorage[bus].data();
            testPool.renderAdd(testOutputs, 4096);
            double energy = 0.0;
            for (std::size_t index = 2048; index < 4096; ++index)
                energy += static_cast<double>(testStorage[0][index]) * testStorage[0][index];
            return static_cast<float>(std::sqrt(energy / 2048.0));
        };

        auto closedFilter = keygroup(60, 7);
        closedFilter.softFilter = 0;
        auto openFilter = closedFilter;
        openFilter.softFilter = 99;
        require(filteredRms(openFilter, 1.0F) > filteredRms(closedFilter, 1.0F) * 20.0F,
                "native base-filter values did not change high-frequency energy");

        auto velocityFilter = closedFilter;
        velocityFilter.softFilter = 50;
        velocityFilter.velocityToFilter = 99;
        require(filteredRms(velocityFilter, 1.0F) >
                    filteredRms(velocityFilter, 20.0F / 127.0F) * 5.0F,
                "velocity-to-filter did not brighten high-velocity playback");

        auto envelopeFilter = closedFilter;
        envelopeFilter.softFilter = 30;
        envelopeFilter.filterEnvelopeAmount = 50;
        envelopeFilter.filterEnvelope = {0, 0, 99, 0};
        auto neutralEnvelope = envelopeFilter;
        neutralEnvelope.filterEnvelopeAmount = 0;
        require(filteredRms(envelopeFilter, 1.0F) >
                    filteredRms(neutralEnvelope, 1.0F) * 10.0F,
                "positive filter envelope amount did not open the filter");

        auto lowTracked = closedFilter;
        lowTracked.softFilter = 50;
        lowTracked.keyboardToFilter = 50;
        lowTracked.constantPitch = true;
        auto highTracked = lowTracked;
        require(filteredRms(highTracked, 1.0F, 72) >
                    filteredRms(lowTracked, 1.0F, 48) * 10.0F,
                "keyboard-to-filter did not track MIDI pitch");

        auto velocityLoudness = keygroup(60, 7);
        velocityLoudness.softFilter = 99;
        velocityLoudness.velocityToLoudness = 99;
        const auto fullVelocityLevel = filteredRms(velocityLoudness, 1.0F);
        const auto lowVelocityLevel =
            filteredRms(velocityLoudness, 20.0F / 127.0F);
        require(fullVelocityLevel > lowVelocityLevel * 6.0F,
                "velocity-to-loudness did not attenuate low-velocity playback");

        velocityLoudness.velocityToLoudness = 0;
        require(std::abs(filteredRms(velocityLoudness, 1.0F) -
                         filteredRms(velocityLoudness, 20.0F / 127.0F)) < 0.0001F,
                "zero velocity-to-loudness changed playback level");

        velocityLoudness.velocityToLoudness = 99;
        velocityLoudness.softLoudness = 50;
        require(std::abs(filteredRms(velocityLoudness, 1.0F) -
                         filteredRms(velocityLoudness, 20.0F / 127.0F)) < 0.0001F,
                "native +50 loudness did not remove velocity dynamics");

        auto psl9013Kick = keygroup(60, 7);
        psl9013Kick.softFilter = 99;
        psl9013Kick.velocityToLoudness = 10;
        psl9013Kick.softLoudness = 20;
        const auto psl9013FullLevel = filteredRms(psl9013Kick, 1.0F, 60, 5);
        const auto psl9013LowLevel =
            filteredRms(psl9013Kick, 20.0F / 127.0F, 60, 5);
        require(std::abs(psl9013FullLevel - 0.75F) < 0.0001F &&
                    psl9013LowLevel < psl9013FullLevel &&
                    psl9013LowLevel > psl9013FullLevel * 0.95F,
                "fixture kick loudness/velocity controls were not combined safely");

        audio::PreparedProgram envelopeProgram;
        auto envelopeSignal = loopingRamp();
        envelopeSignal.samples12 = {1024, 1024};
        envelopeSignal.playbackEnd = envelopeSignal.samples12.size();
        envelopeProgram.samples.push_back(std::move(envelopeSignal));
        auto envelopeKeygroup = keygroup(60, 7);
        envelopeKeygroup.softFilter = 99;
        envelopeKeygroup.amplitudeEnvelope = {50, 0, 99, 50};
        envelopeProgram.keygroups.push_back(envelopeKeygroup);
        audio::ProgramVoicePool envelopePool;
        envelopePool.setProgram(std::move(envelopeProgram));
        envelopePool.setHostSampleRate(1'000.0);
        envelopePool.noteOn(60, 201, 1.0F);
        std::array<std::array<float, 2048>, audio::ProgramVoicePool::outputCount>
            envelopeStorage {};
        audio::ProgramVoicePool::OutputBuffers envelopeOutputs {};
        for (std::size_t bus = 0; bus < envelopeOutputs.size(); ++bus)
            envelopeOutputs[bus] = envelopeStorage[bus].data();
        envelopePool.renderAdd(envelopeOutputs, 1500);
        require(std::abs(envelopeStorage[0][10]) < std::abs(envelopeStorage[0][1200]) * 0.2F,
                "amplitude attack did not rise over its native time range");
        envelopePool.noteOff(60, 201);
        require(envelopePool.isNoteActive(60, 201),
                "non-zero amplitude release stopped immediately");
        envelopeStorage = {};
        envelopePool.renderAdd(envelopeOutputs, 1000);
        require(envelopePool.activeVoiceCount() == 0,
                "amplitude release did not finish and retire its voice");

        std::cout << "PLAY950 program voice-pool tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PLAY950 program voice-pool tests failed: " << error.what() << '\n';
        return 1;
    }
}
