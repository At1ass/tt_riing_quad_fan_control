#ifndef __NVIDIA_HPP__
#define __NVIDIA_HPP__

#include "nvml.h"
#include "system/gpu.hpp"

namespace sys {

class Nvidia : public GPU {
   public:
    Nvidia();
    Nvidia(Nvidia const&) = default;
    Nvidia(Nvidia&&) = delete;
    Nvidia& operator=(Nvidia const&) = default;
    Nvidia& operator=(Nvidia&&) = delete;
    ~Nvidia() override;
    unsigned int getGPUTemp() override;
    std::string getGPUName() override;
    bool readGPUTemp(unsigned int& temp) override;

   private:
    void* library_ = nullptr;
    bool load();
    void cleanUp();
    decltype(&::nvmlInit_v2) nvmlInit_v2 = nullptr;
    decltype(&::nvmlErrorString) nvmlErrorString = nullptr;
    decltype(&::nvmlDeviceGetHandleByIndex_v2) nvmlDeviceGetHandleByIndex_v2 =
        nullptr;
    decltype(&::nvmlDeviceGetName) nvmlDeviceGetName = nullptr;
    decltype(&::nvmlDeviceGetTemperature) nvmlDeviceGetTemperature = nullptr;
    decltype(&::nvmlShutdown) nvmlShutdown = nullptr;
    nvmlDevice_t device{};
};

}  // namespace sys
#endif  // !__NVIDIA_HPP__
