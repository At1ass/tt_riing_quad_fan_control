#include "core/observers/ui/uiObserver.hpp"

namespace core {

void ObserverUiCPU::onEvent(Event const& event) {
    if (event.type == EventType::CPU_TEMP_CHANGED) {
        gui_manager->updateCPUCurrentTemp(event.value);
    }
}

void ObserverUiGPU::onEvent(Event const& event) {
    if (event.type == EventType::GPU_TEMP_CHANGED) {
        gui_manager->updateGPUCurrentTemp(event.value);
    }
}

}  // namespace core
