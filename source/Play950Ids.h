#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace e45recordings::play950 {

enum ParameterId : Steinberg::Vst::ParamID {
    pitchBendParameter = 1000,
    pitchBendRangeParameter = 1001,
    midiReceiveModeParameter = 1002,
    basicMidiChannelParameter = 1003,
    additionalPitchBendParameterBase = 1100,
};

constexpr Steinberg::Vst::ParamID pitchBendParameterForChannel(int channel) noexcept {
    return channel == 0
        ? pitchBendParameter
        : additionalPitchBendParameterBase + static_cast<Steinberg::Vst::ParamID>(channel - 1);
}

constexpr int channelForPitchBendParameter(Steinberg::Vst::ParamID parameter) noexcept {
    if (parameter == pitchBendParameter)
        return 0;
    if (parameter >= additionalPitchBendParameterBase &&
        parameter < additionalPitchBendParameterBase + 15)
        return static_cast<int>(parameter - additionalPitchBendParameterBase) + 1;
    return -1;
}

static const Steinberg::FUID ProcessorUID (0xE45A0950, 0x13A64B21, 0xA99510FE, 0x81000101);
static const Steinberg::FUID ControllerUID (0xE45A0950, 0x13A64B21, 0xA99510FE, 0x81000102);

} // namespace e45recordings::play950
