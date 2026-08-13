#include "Play950Controller.h"
#include "Play950Ids.h"
#include "Play950Processor.h"
#include "Play950Version.h"

#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace e45recordings::play950;

BEGIN_FACTORY_DEF(PLAY950_COMPANY_NAME, PLAY950_COMPANY_WEB, PLAY950_COMPANY_EMAIL)

DEF_CLASS2(INLINE_UID_FROM_FUID(ProcessorUID), PClassInfo::kManyInstances,
           kVstAudioEffectClass, PLAY950_PLUGIN_NAME, Vst::kDistributable,
           "Instrument|Sampler", PLAY950_VERSION, kVstVersionString,
           Processor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(ControllerUID), PClassInfo::kManyInstances,
           kVstComponentControllerClass, PLAY950_PLUGIN_NAME " Controller", 0,
           "", PLAY950_VERSION, kVstVersionString, Controller::createInstance)

END_FACTORY

