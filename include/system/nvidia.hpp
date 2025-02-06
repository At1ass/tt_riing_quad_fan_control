#ifndef __NVIDIA_HPP__
#define __NVIDIA_HPP__

#include "nvml.h"
#include "system/gpu.hpp"

namespace sys {
    class Nvidia : public GPU {
        public:
            Nvidia();
            ~Nvidia();
            float getGPUTemp() override;
            std::string getGPUName() override;
            bool readGPUTemp(unsigned int& temp) override;
        private:
            void *library_;
            bool Load();
            void CleanUp();
            decltype(&::nvmlInit_v2) nvmlInit_v2;
            decltype(&::nvmlErrorString) nvmlErrorString;
            decltype(&::nvmlDeviceGetHandleByIndex_v2) nvmlDeviceGetHandleByIndex_v2;
            decltype(&::nvmlDeviceGetName) nvmlDeviceGetName;
            decltype(&::nvmlDeviceGetTemperature) nvmlDeviceGetTemperature;
            decltype(&::nvmlShutdown) nvmlShutdown;
            nvmlDevice_t device{};
    };
}

#endif // !__NVIDIA_HPP__
