#include "Play950Controller.h"

#include "Play950Editor.h"
#include "Play950Messages.h"
#include "Play950Ids.h"
#include "formats/P9.h"
#include "workflow/ImageWorkflow.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

constexpr std::size_t maximumStreamStateBytes = 64U * 1024U * 1024U;

std::vector<std::byte> readStateStream(Steinberg::IBStream* stream) {
    if (!stream)
        throw std::runtime_error("null project-state stream");
    std::vector<std::byte> bytes;
    std::array<std::byte, 64U * 1024U> buffer {};
    while (bytes.size() <= maximumStreamStateBytes) {
        Steinberg::int32 bytesRead = 0;
        const auto result = stream->read(buffer.data(), static_cast<Steinberg::int32>(buffer.size()),
                                         &bytesRead);
        if (bytesRead < 0 || bytesRead > static_cast<Steinberg::int32>(buffer.size()))
            throw std::runtime_error("invalid project-state stream read");
        bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + bytesRead);
        if (bytes.size() > maximumStreamStateBytes)
            throw std::runtime_error("project-state stream is too large");
        if (bytesRead == 0 || result != Steinberg::kResultOk)
            break;
    }
    return bytes;
}

} // namespace

namespace e45recordings::play950 {

Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context) {
    const auto result = EditController::initialize(context);
    if (result != Steinberg::kResultOk)
        return result;
    parameters.addParameter(STR16("Pitch Bend"), nullptr, 0, 0.5,
                            Steinberg::Vst::ParameterInfo::kIsHidden,
                            pitchBendParameter);
    for (int channel = 1; channel < 16; ++channel) {
        Steinberg::UString128 title("Pitch Bend Ch ");
        Steinberg::UString128 number;
        number.printInt(channel + 1);
        title.append(number);
        parameters.addParameter(title, nullptr, 0, 0.5,
                                Steinberg::Vst::ParameterInfo::kIsHidden,
                                pitchBendParameterForChannel(channel));
    }
    parameters.addParameter(new Steinberg::Vst::RangeParameter(
        STR16("Pitch Bend Range"), pitchBendRangeParameter, STR16("st"),
        1.0, 12.0, 2.0, 11, Steinberg::Vst::ParameterInfo::kCanAutomate));
    auto* midiReceive = new Steinberg::Vst::StringListParameter(
        STR16("MIDI Receive"), midiReceiveModeParameter);
    midiReceive->appendString(STR16("Omni"));
    midiReceive->appendString(STR16("Keygroup Channels"));
    parameters.addParameter(midiReceive);
    parameters.addParameter(new Steinberg::Vst::RangeParameter(
        STR16("Basic MIDI Channel"), basicMidiChannelParameter, nullptr,
        1.0, 16.0, 1.0, 15, Steinberg::Vst::ParameterInfo::kCanAutomate));
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Controller::getMidiControllerAssignment(
    Steinberg::int32 busIndex, Steinberg::int16 channel,
    Steinberg::Vst::CtrlNumber midiControllerNumber, Steinberg::Vst::ParamID& id) {
    if (busIndex == 0 && channel >= 0 && channel < 16 &&
        midiControllerNumber == Steinberg::Vst::kPitchBend) {
        id = pitchBendParameterForChannel(channel);
        return Steinberg::kResultTrue;
    }
    return Steinberg::kResultFalse;
}

int Controller::pitchBendRangeSemitones() noexcept {
    return std::clamp(static_cast<int>(std::lround(
        1.0 + getParamNormalized(pitchBendRangeParameter) * 11.0)), 1, 12);
}

void Controller::setPitchBendRangeSemitones(int semitones) {
    const auto normalized = static_cast<double>(std::clamp(semitones, 1, 12) - 1) / 11.0;
    beginEdit(pitchBendRangeParameter);
    setParamNormalized(pitchBendRangeParameter, normalized);
    performEdit(pitchBendRangeParameter, normalized);
    endEdit(pitchBendRangeParameter);
}

bool Controller::midiOmni() noexcept {
    return getParamNormalized(midiReceiveModeParameter) < 0.5;
}

void Controller::setMidiOmni(bool omni) {
    const auto normalized = omni ? 0.0 : 1.0;
    beginEdit(midiReceiveModeParameter);
    setParamNormalized(midiReceiveModeParameter, normalized);
    performEdit(midiReceiveModeParameter, normalized);
    endEdit(midiReceiveModeParameter);
}

int Controller::basicMidiChannel() noexcept {
    return std::clamp(static_cast<int>(std::lround(
        1.0 + getParamNormalized(basicMidiChannelParameter) * 15.0)), 1, 16);
}

void Controller::setBasicMidiChannel(int channel) {
    const auto normalized = static_cast<double>(std::clamp(channel, 1, 16) - 1) / 15.0;
    beginEdit(basicMidiChannelParameter);
    setParamNormalized(basicMidiChannelParameter, normalized);
    performEdit(basicMidiChannelParameter, normalized);
    endEdit(basicMidiChannelParameter);
}

Steinberg::tresult PLUGIN_API Controller::setComponentState(Steinberg::IBStream* state) {
    try {
        auto restored = state::deserializeProjectState(readStateStream(state));
        setParamNormalized(pitchBendRangeParameter,
            static_cast<double>(restored.pitchBendRangeSemitones - 1) / 11.0);
        setParamNormalized(midiReceiveModeParameter, restored.midiOmni ? 0.0 : 1.0);
        setParamNormalized(basicMidiChannelParameter,
            static_cast<double>(restored.basicMidiChannel - 1) / 15.0);
        std::vector<content::LoadedProgram> programs;
        if (!restored.browserPrograms.empty()) {
            programs.reserve(restored.browserPrograms.size());
            for (auto& embedded : restored.browserPrograms) {
                state::ProjectState programState;
                programState.p9 = std::move(embedded.p9);
                programState.s9Samples = std::move(embedded.s9Samples);
                const auto parsed = formats::parseP9(programState.p9);
                programs.push_back({parsed.name.empty() ? embedded.fileName : parsed.name,
                                    std::move(embedded.fileName), std::move(programState)});
            }
            sourceName_ = std::move(restored.sourceName);
            sourcePath_ = std::move(restored.sourcePath);
            selectedProgram_ = restored.selectedProgramIndex;
        } else if (restored.hasProgram()) {
            const auto parsed = formats::parseP9(restored.p9);
            auto name = parsed.name.empty() ? std::string("Embedded program") : parsed.name;
            programs.push_back({name, name + ".P9", std::move(restored)});
            sourceName_ = "embedded program";
            sourcePath_.clear();
            selectedProgram_ = 0;
        } else {
            sourceName_.clear();
            sourcePath_.clear();
            selectedProgram_ = 0;
        }
        programs_ = std::move(programs);
        restoredFromHostState_ = !programs_.empty();
        statusText_ = programs_.empty() ? "No program loaded." : restoredStatus(selectedProgram_);
        return Steinberg::kResultOk;
    } catch (...) {
        programs_.clear();
        sourceName_.clear();
        sourcePath_.clear();
        selectedProgram_ = 0;
        restoredFromHostState_ = false;
        statusText_ = "The program stored in the Ableton Set could not be restored.";
        return Steinberg::kResultFalse;
    }
}

Steinberg::IPlugView* PLUGIN_API Controller::createView(Steinberg::FIDString name) {
    if (Steinberg::FIDStringsEqual(name, Steinberg::Vst::ViewType::kEditor))
        return createPlay950Editor(*this);
    return nullptr;
}

bool Controller::setAvailablePrograms(std::vector<content::LoadedProgram> programs,
                                      std::string sourceName, std::string sourcePath) {
    if (programs.empty())
        return false;
    auto previousPrograms = std::move(programs_);
    auto previousSourceName = std::move(sourceName_);
    auto previousSourcePath = std::move(sourcePath_);
    const auto previousSelection = selectedProgram_;
    const auto previousRestoredState = restoredFromHostState_;
    programs_ = std::move(programs);
    sourceName_ = std::move(sourceName);
    sourcePath_ = std::move(sourcePath);
    selectedProgram_ = 0;
    restoredFromHostState_ = false;
    if (sendProjectState(projectStateForSelection(0), selectionStatus(0))) {
        liveEditProgramFilename_.clear();
        liveEditBaselineP9_.clear();
        liveEditRevision_ = 0;
        return true;
    }
    programs_ = std::move(previousPrograms);
    sourceName_ = std::move(previousSourceName);
    sourcePath_ = std::move(previousSourcePath);
    selectedProgram_ = previousSelection;
    restoredFromHostState_ = previousRestoredState;
    return false;
}

bool Controller::reloadAvailablePrograms(std::vector<content::LoadedProgram> programs) {
    if (programs.empty() || sourcePath_.empty())
        return false;
    const std::string selectedFile = selectedProgram_ < programs_.size()
        ? programs_[selectedProgram_].fileName : std::string {};
    std::vector<std::string> newFileNames;
    newFileNames.reserve(programs.size());
    for (const auto& program : programs)
        newFileNames.push_back(program.fileName);
    const auto newSelection = workflow::selectionAfterReload(newFileNames, selectedFile);

    auto previousPrograms = std::move(programs_);
    const auto previousSelection = selectedProgram_;
    const auto previousRestoredState = restoredFromHostState_;
    programs_ = std::move(programs);
    selectedProgram_ = newSelection;
    restoredFromHostState_ = false;
    if (sendProjectState(projectStateForSelection(newSelection),
                         "Reloaded " + sourceName_ + " — " +
                             programDisplayName(newSelection))) {
        liveEditProgramFilename_.clear();
        liveEditBaselineP9_.clear();
        liveEditRevision_ = 0;
        return true;
    }
    programs_ = std::move(previousPrograms);
    selectedProgram_ = previousSelection;
    restoredFromHostState_ = previousRestoredState;
    return false;
}

bool Controller::selectProgram(std::size_t index) {
    if (index >= programs_.size())
        return false;
    if (!sendProjectState(projectStateForSelection(index), selectionStatus(index)))
        return false;
    selectedProgram_ = index;
    return true;
}

bool Controller::currentProgramForEditing(
    std::string& fileName,
    std::vector<std::byte>& p9Data,
    std::vector<std::byte>& baselineP9Data) const {
    if (selectedProgram_ >= programs_.size())
        return false;
    fileName = programs_[selectedProgram_].fileName;
    p9Data = programs_[selectedProgram_].state.p9;
    baselineP9Data = liveEditBaselineP9_.empty()
        ? p9Data : liveEditBaselineP9_;
    return !fileName.empty() && !p9Data.empty() && !baselineP9Data.empty();
}

void Controller::beginLiveEditSession(std::string identifier) {
    const auto selectedFilename = selectedProgram_ < programs_.size()
        ? programs_[selectedProgram_].fileName : std::string {};
    const bool isSameSession = identifier == liveEditSessionIdentifier_
        && selectedFilename == liveEditProgramFilename_
        && !liveEditBaselineP9_.empty();
    liveEditSessionIdentifier_ = std::move(identifier);
    liveEditProgramFilename_ = selectedFilename;
    if (!isSameSession) {
        liveEditRevision_ = 0;
        liveEditBaselineP9_ = selectedProgram_ < programs_.size()
            ? programs_[selectedProgram_].state.p9 : std::vector<std::byte> {};
    }
}

bool Controller::applyEditorProgram(std::vector<std::byte> p9Data,
                                    std::uint64_t revision) {
    const auto target = std::find_if(
        programs_.begin(), programs_.end(), [&](const content::LoadedProgram& program) {
            return program.fileName == liveEditProgramFilename_;
        });
    if (target == programs_.end() || revision <= liveEditRevision_)
        return false;
    try {
        const auto parsed = formats::parseP9(p9Data);
        const auto targetIndex = static_cast<std::size_t>(
            std::distance(programs_.begin(), target));
        auto& selected = *target;
        auto previousData = std::move(selected.state.p9);
        auto previousName = selected.nativeName;
        selected.state.p9 = std::move(p9Data);
        if (!parsed.name.empty())
            selected.nativeName = parsed.name;
        if (!sendProgramUpdate(
                selected.state.p9,
                "Auditioned EDIT950 revision " + std::to_string(revision))) {
            selected.state.p9 = std::move(previousData);
            selected.nativeName = std::move(previousName);
            return false;
        }
        selectedProgram_ = targetIndex;
        liveEditRevision_ = revision;
        return true;
    } catch (...) {
        return false;
    }
}

state::ProjectState Controller::projectStateForSelection(std::size_t index) {
    if (index >= programs_.size())
        throw std::out_of_range("program selection is out of range");
    auto result = programs_[index].state;
    result.pitchBendRangeSemitones = static_cast<std::uint32_t>(pitchBendRangeSemitones());
    result.midiOmni = midiOmni();
    result.basicMidiChannel = static_cast<std::uint32_t>(basicMidiChannel());
    result.sourceName = sourceName_;
    result.sourcePath = sourcePath_;
    result.browserPrograms.reserve(programs_.size());
    for (const auto& program : programs_)
        result.browserPrograms.push_back(
            {program.fileName, program.state.p9, program.state.s9Samples});
    result.selectedProgramIndex = static_cast<std::uint32_t>(index);
    return result;
}

std::string Controller::programDisplayName(std::size_t index) const {
    if (index >= programs_.size())
        return {};
    const auto& program = programs_[index];
    const bool duplicateName = std::count_if(
        programs_.begin(), programs_.end(), [&](const content::LoadedProgram& candidate) {
            return candidate.nativeName == program.nativeName;
        }) > 1;
    return duplicateName ? program.nativeName + " — " + program.fileName : program.nativeName;
}

std::string Controller::selectionStatus(std::size_t index) const {
    if (restoredFromHostState_)
        return restoredStatus(index);
    if (index >= programs_.size())
        return {};
    if (programs_.size() == 1)
        return "Loaded " + sourceName_;
    return "Loaded " + sourceName_ + " — " + programDisplayName(index);
}

std::string Controller::restoredStatus(std::size_t index) const {
    if (index >= programs_.size())
        return "Program collection restored from the Ableton Set.";
    if (programs_.size() == 1)
        return "Restored " + sourceName_ + " from the Ableton Set.";
    return "Restored " + sourceName_ + " — " + programDisplayName(index) +
           " from the Ableton Set.";
}

bool Controller::sendProjectState(const state::ProjectState& projectState,
                                  std::string statusText) {
    try {
        const auto bytes = state::serializeProjectState(projectState);
        if (bytes.size() > std::numeric_limits<Steinberg::uint32>::max())
            return false;
        auto message = Steinberg::owned(allocateMessage());
        if (!message)
            return false;
        message->setMessageID(loadProjectStateMessage);
        if (!message->getAttributes() ||
            message->getAttributes()->setBinary(
                projectStateAttribute, bytes.data(), static_cast<Steinberg::uint32>(bytes.size())) !=
                Steinberg::kResultOk)
            return false;
        if (sendMessage(message) != Steinberg::kResultOk)
            return false;
        statusText_ = std::move(statusText);
        (void)setDirty(true);
        return true;
    } catch (...) {
        return false;
    }
}

bool Controller::sendProgramUpdate(
    const std::vector<std::byte>& p9Data,
    std::string statusText) {
    if (p9Data.empty() ||
        p9Data.size() > std::numeric_limits<Steinberg::uint32>::max())
        return false;
    auto message = Steinberg::owned(allocateMessage());
    if (!message)
        return false;
    message->setMessageID(updateProgramMessage);
    if (!message->getAttributes() ||
        message->getAttributes()->setBinary(
            programDataAttribute,
            p9Data.data(),
            static_cast<Steinberg::uint32>(p9Data.size())) != Steinberg::kResultOk)
        return false;
    if (sendMessage(message) != Steinberg::kResultOk)
        return false;
    statusText_ = std::move(statusText);
    (void)setDirty(true);
    return true;
}

} // namespace e45recordings::play950
