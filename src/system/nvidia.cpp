#include "system/nvidia.hpp"
#include "core/logger.hpp"
#include "nvml.h"
#include <stdexcept>
#include <string>
#include <dlfcn.h>

namespace sys {
    bool Nvidia::Load() {
        std::string library_name = "libnvidia-ml.so.1";
        library_ = dlopen(library_name.c_str(), RTLD_LAZY);

        nvmlInit_v2 = reinterpret_cast<decltype(this->nvmlInit)>(
                dlsym(library_, "nvmlInit"));
        if (!nvmlInit_v2) {
            CleanUp();
            return false;
        }

        nvmlShutdown = reinterpret_cast<decltype(this->nvmlShutdown)>(
                dlsym(library_, "nvmlShutdown"));
        if (!nvmlShutdown) {
            CleanUp();
            return false;
        }

        nvmlDeviceGetName = reinterpret_cast<decltype(this->nvmlDeviceGetName)>(
                dlsym(library_, "nvmlDeviceGetName"));
        if (!nvmlDeviceGetName) {
            CleanUp();
            return false;
        }

        nvmlDeviceGetTemperature = reinterpret_cast<decltype(this->nvmlDeviceGetTemperature)>(
                dlsym(library_, "nvmlDeviceGetTemperature"));
        if (!nvmlDeviceGetTemperature) {
            CleanUp();
            return false;
        }

        nvmlDeviceGetHandleByIndex_v2 = reinterpret_cast<decltype(this->nvmlDeviceGetHandleByIndex_v2)>(
                dlsym(library_, "nvmlDeviceGetHandleByIndex_v2"));
        if (!nvmlDeviceGetHandleByIndex_v2) {
            CleanUp();
            return false;
        }

        nvmlErrorString = reinterpret_cast<decltype(this->nvmlErrorString)>(
                dlsym(library_, "nvmlErrorString"));
        if (!nvmlErrorString) {
            CleanUp();
            return false;
        }

        core::Logger::log_(core::LogLevel::INFO) << "Opening NVML Success" << std::endl;

        return true;
    }

    void Nvidia::CleanUp() {
        nvmlInit_v2 = NULL;
        nvmlShutdown = NULL;
        nvmlDeviceGetTemperature = NULL;
        nvmlDeviceGetHandleByIndex_v2 = NULL;
        nvmlErrorString = NULL;
        nvmlDeviceGetName = NULL;
    }

    Nvidia::Nvidia() {
        if (Load() != true) {
            throw std::runtime_error("Cannot open NVML library");
        }

        nvmlReturn_t result;
        char name[NVML_DEVICE_NAME_BUFFER_SIZE];
        int ret = 0;

        result = nvmlInit();
        if (NVML_SUCCESS != result) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to initialize NVML: " << nvmlErrorString(result) << '\n';
            throw std::runtime_error("Failed to initialize NVML");
        }

        result = nvmlDeviceGetHandleByIndex(0, &device);
        if (NVML_SUCCESS != result) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to get handle for device: " << nvmlErrorString(result) << '\n';
            throw std::runtime_error("Failed to get handle for device");
        }

        result = nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
        if (NVML_SUCCESS != result) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to get name of device:" << nvmlErrorString(result) << '\n';
            throw std::runtime_error("Failed to get name of device:");
        }

        gpu_name = std::string(name);
    }

    auto Nvidia::getGPUName() -> std::string {
        return gpu_name;
    }

    auto Nvidia::getGPUTemp() -> float {
        return gpu_temp;
    }

    auto Nvidia::readGPUTemp(unsigned int& temp) -> bool {
        nvmlReturn_t result;

        result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
        if (NVML_SUCCESS != result) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to get gpu temp: " << nvmlErrorString(result) << '\n';
            return false;
        }

        return true;
    }

    Nvidia::~Nvidia() {
        nvmlShutdown();
        dlclose(library_);
        CleanUp();
    }

}
