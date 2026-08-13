#pragma once

namespace Steinberg {
class IPlugView;
}

namespace e45recordings::play950 {

class Controller;
[[nodiscard]] Steinberg::IPlugView* createPlay950Editor(Controller& controller);

} // namespace e45recordings::play950
