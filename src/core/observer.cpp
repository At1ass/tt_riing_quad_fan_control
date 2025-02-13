#include "core/observer.hpp"

namespace core {
void ObserverCPU::onEvent(Event const& event) {
    if (event.type == EventType::CPU_TEMP_CHANGED) {
        fan_controller->updateCPUfans(event.value);
    }
}

void ObserverGPU::onEvent(Event const& event) {
    if (event.type == EventType::GPU_TEMP_CHANGED) {
        fan_controller->updateGPUfans(event.value);
    }
}
}  // namespace core
