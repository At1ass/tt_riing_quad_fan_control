#include "system/GPUController.hpp"
#include "core/logger.hpp"
#include "system/amd.hpp"
#include "system/dummyGPU.hpp"
#include "system/nvidia.hpp"
#include <filesystem>
#include <fstream>

namespace sys {
    GPUController::GPUController() {
        namespace fs = std::filesystem;
        try {
            for (const auto &entry : fs::directory_iterator("/sys/class/drm")) {
                // Обычно нас интересуют директории вида card0, card1 и т.д.
                if (entry.is_directory() && entry.path().filename().string().rfind("card", 0) == 0) {
                    // Пути вида /sys/class/drm/card0/device/vendor
                    auto vendorFile = entry.path() / "device" / "vendor";
                    if (fs::exists(vendorFile)) {
                        std::ifstream fin(vendorFile);
                        if (fin.is_open()) {
                            std::string vendorId;
                            fin >> vendorId; // например, "0x10de"
                            if (vendorId == "0x10de") {
                                core::Logger::log_(core::LogLevel::INFO) << entry.path().filename().string()
                                    << " -> NVIDIA\n";
                                gpu = std::make_unique<Nvidia>();
                            } else if (vendorId == "0x1002" || vendorId == "0x1022") {
                                core::Logger::log_(core::LogLevel::ERROR) << entry.path().filename().string()
                                    << " -> AMD\n";
                                gpu = std::make_unique<AMD>(entry.path().filename().string());
                            } else if (vendorId == "0x8086") {
                                core::Logger::log_(core::LogLevel::WARNING) << entry.path().filename().string()
                                    << " -> Intel\n";
                                core::Logger::log_(core::LogLevel::WARNING) << "Intel GPU not supported. Set dummy GPU" << std::endl;
                                gpu = std::make_unique<DummyGPU>();
                            } else {
                                core::Logger::log_(core::LogLevel::WARNING)<< entry.path().filename().string()
                                    << " -> Unknown vendor: " << vendorId << "\n";
                                core::Logger::log_(core::LogLevel::WARNING) << "Unknown GPU. Set dummy GPU" << std::endl;
                                gpu = std::make_unique<DummyGPU>();
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            core::Logger::log_(core::LogLevel::INFO) << "Failed initialize GPU: " << e.what() << std::endl;

        }
    }

    std::string GPUController::getGPUName() {
        return gpu->getGPUName();
    }

    float GPUController::getGPUTemp() {
        return gpu->getGPUTemp();
    }

    bool GPUController::readGPUTemp(unsigned int &temp) {
        return gpu->readGPUTemp(temp);
    }
}
