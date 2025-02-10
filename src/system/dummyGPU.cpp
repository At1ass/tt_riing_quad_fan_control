#include "system/dummyGPU.hpp"

namespace sys {
    DummyGPU::DummyGPU() {
        gpu_name = "Unknown GPU";
        gpu_temp = 50;
    }

    float DummyGPU::getGPUTemp() {
        return gpu_temp;
    }
    std::string DummyGPU::getGPUName() {
        return gpu_name;
    }
    bool DummyGPU::readGPUTemp(unsigned int& temp) {
        return true;
    }
}
