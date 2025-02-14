#include "system/gpus/nvidia.hpp"

#include <dlfcn.h>

#include <stdexcept>
#include <string>

#include "core/logger.hpp"
#include "nvml.h"

namespace sys {

bool Nvidia::load() {
    std::string library_name = "libnvidia-ml.so.1";
    library_ = dlopen(library_name.c_str(), RTLD_LAZY);

    nvmlInit_v2 = reinterpret_cast<decltype(this->nvmlInit)>( // NOLINT
        dlsym(library_, "nvmlInit"));
    if (!nvmlInit_v2) {
        cleanUp();
        return false;
    }

    nvmlShutdown = reinterpret_cast<decltype(this->nvmlShutdown)>(  // NOLINT
        dlsym(library_, "nvmlShutdown"));
    if (!nvmlShutdown) {
        cleanUp();
        return false;
    }

    nvmlDeviceGetName =
        reinterpret_cast<decltype(this->nvmlDeviceGetName)>(  // NOLINT
            dlsym(library_, "nvmlDeviceGetName"));
    if (!nvmlDeviceGetName) {
        cleanUp();
        return false;
    }

    nvmlDeviceGetTemperature =
        reinterpret_cast<decltype(this->nvmlDeviceGetTemperature)>(  // NOLINT
            dlsym(library_, "nvmlDeviceGetTemperature"));
    if (!nvmlDeviceGetTemperature) {
        cleanUp();
        return false;
    }

    nvmlDeviceGetHandleByIndex_v2 = reinterpret_cast< //NOLINT
        decltype(this->nvmlDeviceGetHandleByIndex_v2)>(  
        dlsym(library_, "nvmlDeviceGetHandleByIndex_v2"));
    if (!nvmlDeviceGetHandleByIndex_v2) {
        cleanUp();
        return false;
    }

    nvmlErrorString =
        reinterpret_cast<decltype(this->nvmlErrorString)>(  // NOLINT
            dlsym(library_, "nvmlErrorString"));
    if (!nvmlErrorString) {
        cleanUp();
        return false;
    }

    core::Logger::log(core::LogLevel::INFO)
        << "Opening NVML Success" << std::endl;

    return true;
}

void Nvidia::cleanUp() {
    nvmlInit_v2 = NULL;
    nvmlShutdown = NULL;
    nvmlDeviceGetTemperature = NULL;
    nvmlDeviceGetHandleByIndex_v2 = NULL;
    nvmlErrorString = NULL;
    nvmlDeviceGetName = NULL;
}

Nvidia::Nvidia() {
    if (load() != true) {
        throw std::runtime_error("Cannot open NVML library");
    }

    nvmlReturn_t result = NVML_SUCCESS;
    // char name[NVML_DEVICE_NAME_BUFFER_SIZE];
    std::array<char, NVML_DEVICE_NAME_BUFFER_SIZE> name{};
    int ret = 0;

    result = nvmlInit();
    if (NVML_SUCCESS != result) {
        core::Logger::log(core::LogLevel::ERROR)
            << "Failed to initialize NVML: " << nvmlErrorString(result) << '\n';
        throw std::runtime_error("Failed to initialize NVML");
    }

    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (NVML_SUCCESS != result) {
        core::Logger::log(core::LogLevel::ERROR)
            << "Failed to get handle for device: " << nvmlErrorString(result)
            << '\n';
        throw std::runtime_error("Failed to get handle for device");
    }

    result =
        nvmlDeviceGetName(device, name.data(), NVML_DEVICE_NAME_BUFFER_SIZE);
    if (NVML_SUCCESS != result) {
        core::Logger::log(core::LogLevel::ERROR)
            << "Failed to get name of device:" << nvmlErrorString(result)
            << '\n';
        throw std::runtime_error("Failed to get name of device:");
    }

    gpu_name = std::string(name.data(), NVML_DEVICE_NAME_BUFFER_SIZE);
}

auto Nvidia::getGPUName() -> std::string { return gpu_name; }

auto Nvidia::getGPUTemp() -> unsigned int { return gpu_temp; }

auto Nvidia::readGPUTemp(unsigned int& temp) -> bool {
    nvmlReturn_t result = NVML_SUCCESS;

    result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
    if (NVML_SUCCESS != result) {
        core::Logger::log(core::LogLevel::ERROR)
            << "Failed to get gpu temp: " << nvmlErrorString(result) << '\n';
        return false;
    }

    return true;
}

Nvidia::~Nvidia() {
    nvmlShutdown();
    dlclose(library_);
    cleanUp();
}

}  // namespace sys
