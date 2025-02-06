#include "core/uiObserver.hpp"

namespace core {
    void ObserverUiCPU::onEvent(const Event &event) {
        if (event.type == EventType::CPU_TEMP_CHANGED) {
            gui_manager->updateCPUCurrentTemp(event.value);
        }
    }

    void ObserverUiGPU::onEvent(const Event &event) {
        if (event.type == EventType::GPU_TEMP_CHANGED) {
            gui_manager->updateGPUCurrentTemp(event.value);
        }
    }
}
