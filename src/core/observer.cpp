#include "core/observer.hpp"

namespace core {
    void ObserverCPU::onEvent(const Event& event) {
        if (event.type == EventType::CPU_TEMP_CHANGED) {
            fan_controller->updateCPUfans(event.value);
        }
    }

    void ObserverGPU::onEvent(const Event& event) {
        if (event.type == EventType::GPU_TEMP_CHANGED) {
            fan_controller->updateGPUfans(event.value);
        }
    }
}
