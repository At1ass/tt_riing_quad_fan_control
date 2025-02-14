#include "system/GPUController.hpp"

#include <filesystem>
#include <fstream>

#include "core/logger.hpp"
#include "system/gpus/amd.hpp"
#include "system/gpus/dummyGPU.hpp"
#include "system/gpus/nvidia.hpp"

namespace sys {

GPUController::GPUController() {
    namespace fs = std::filesystem;
    try {
        for (auto const& entry : fs::directory_iterator("/sys/class/drm")) {
            // Обычно нас интересуют директории вида card0, card1 и т.д.
            if (entry.is_directory() &&
                entry.path().filename().string().rfind("card", 0) == 0) {
                // Пути вида /sys/class/drm/card0/device/vendor
                auto vendor_file = entry.path() / "device" / "vendor";
                if (fs::exists(vendor_file)) {
                    std::ifstream fin(vendor_file);
                    if (fin.is_open()) {
                        std::string vendor_id;
                        fin >> vendor_id;  // например, "0x10de"
                        if (vendor_id == "0x10de") {
                            core::Logger::log(core::LogLevel::INFO)
                                << entry.path().filename().string()
                                << " -> NVIDIA\n";
                            gpu = std::make_unique<Nvidia>();
                        } else if (vendor_id == "0x1002" ||
                                   vendor_id == "0x1022") {
                            core::Logger::log(core::LogLevel::ERROR)
                                << entry.path().filename().string()
                                << " -> AMD\n";
                            gpu = std::make_unique<AMD>(
                                entry.path().filename().string());
                        } else if (vendor_id == "0x8086") {
                            core::Logger::log(core::LogLevel::WARNING)
                                << entry.path().filename().string()
                                << " -> Intel\n";
                            core::Logger::log(core::LogLevel::WARNING)
                                << "Intel GPU not supported. Set dummy GPU"
                                << std::endl;
                            gpu = std::make_unique<DummyGPU>();
                        } else {
                            core::Logger::log(core::LogLevel::WARNING)
                                << entry.path().filename().string()
                                << " -> Unknown vendor: " << vendor_id << "\n";
                            core::Logger::log(core::LogLevel::WARNING)
                                << "Unknown GPU. Set dummy GPU" << std::endl;
                            gpu = std::make_unique<DummyGPU>();
                        }
                    }
                }
            }
        }
    } catch (std::exception const& e) {
        core::Logger::log(core::LogLevel::INFO)
            << "Failed initialize GPU: " << e.what() << std::endl;
    }
}

std::string GPUController::getGPUName() { return gpu->getGPUName(); }

float GPUController::getGPUTemp() { return gpu->getGPUTemp(); }

bool GPUController::readGPUTemp(unsigned int& temp) {
    return gpu->readGPUTemp(temp);
}

}  // namespace sys
