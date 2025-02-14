#include "system/gpus/dummyGPU.hpp"

constexpr int const DEFAULT_SPEED = 50;

namespace sys {

DummyGPU::DummyGPU() {
    gpu_name = "Unknown GPU";
    gpu_temp = DEFAULT_SPEED;
}

auto DummyGPU::getGPUTemp() -> unsigned int { return gpu_temp; }
auto DummyGPU::getGPUName() -> std::string { return gpu_name; }
bool DummyGPU::readGPUTemp(unsigned int& temp) { return true; }

}  // namespace sys
