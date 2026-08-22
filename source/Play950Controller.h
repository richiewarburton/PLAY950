#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include "content/ContentLoader.h"
#include "state/ProjectState.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace e45recordings::play950 {

class Controller final : public Steinberg::Vst::EditController,
                         public Steinberg::Vst::IMidiMapping {
public:
    static Steinberg::FUnknown* createInstance(void*) { return static_cast<Steinberg::Vst::IEditController*>(new Controller); }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override;
    Steinberg::tresult PLUGIN_API getMidiControllerAssignment(
        Steinberg::int32 busIndex, Steinberg::int16 channel,
        Steinberg::Vst::CtrlNumber midiControllerNumber,
        Steinberg::Vst::ParamID& id) override;

    OBJ_METHODS(Controller, EditController)
    DEFINE_INTERFACES
        DEF_INTERFACE(Steinberg::Vst::IMidiMapping)
    END_DEFINE_INTERFACES(EditController)
    REFCOUNT_METHODS(EditController)

    bool setAvailablePrograms(std::vector<content::LoadedProgram> programs,
                              std::string sourceName, std::string sourcePath = {});
    bool reloadAvailablePrograms(std::vector<content::LoadedProgram> programs);
    bool selectProgram(std::size_t index);
    [[nodiscard]] std::size_t programCount() const noexcept { return programs_.size(); }
    [[nodiscard]] std::size_t selectedProgramIndex() const noexcept { return selectedProgram_; }
    [[nodiscard]] std::string programDisplayName(std::size_t index) const;
    [[nodiscard]] bool currentProgramForEditing(
        std::string& fileName,
        std::vector<std::byte>& p9Data,
        std::vector<std::byte>& baselineP9Data) const;
    [[nodiscard]] const std::string& liveEditSessionIdentifier() const noexcept {
        return liveEditSessionIdentifier_;
    }
    [[nodiscard]] bool hasActiveLiveEditSession() const noexcept {
        return !liveEditSessionIdentifier_.empty()
            && !liveEditProgramFilename_.empty();
    }
    void beginLiveEditSession(std::string identifier);
    [[nodiscard]] std::uint64_t liveEditRevision() const noexcept {
        return liveEditRevision_;
    }
    bool applyEditorProgram(std::vector<std::byte> p9Data, std::uint64_t revision);
    [[nodiscard]] const std::string& statusText() const noexcept { return statusText_; }
    [[nodiscard]] const std::string& sourcePath() const noexcept { return sourcePath_; }
    [[nodiscard]] int pitchBendRangeSemitones() noexcept;
    void setPitchBendRangeSemitones(int semitones);
    [[nodiscard]] bool midiOmni() noexcept;
    void setMidiOmni(bool omni);
    [[nodiscard]] int basicMidiChannel() noexcept;
    void setBasicMidiChannel(int channel);

private:
    [[nodiscard]] state::ProjectState projectStateForSelection(std::size_t index);
    bool sendProjectState(const state::ProjectState& projectState, std::string statusText);
    bool sendProgramUpdate(
        const std::vector<std::byte>& p9Data,
        std::string statusText);
    [[nodiscard]] std::string selectionStatus(std::size_t index) const;
    [[nodiscard]] std::string restoredStatus(std::size_t index) const;

    std::vector<content::LoadedProgram> programs_;
    std::string sourceName_;
    std::string sourcePath_;
    std::size_t selectedProgram_ {0};
    std::string liveEditSessionIdentifier_;
    std::string liveEditProgramFilename_;
    std::vector<std::byte> liveEditBaselineP9_;
    std::uint64_t liveEditRevision_ {0};
    bool restoredFromHostState_ {false};
    std::string statusText_ {"No program loaded."};
};

} // namespace e45recordings::play950
