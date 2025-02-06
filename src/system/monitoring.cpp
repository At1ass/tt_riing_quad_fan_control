#include "system/monitoring.hpp"
#include "core/logger.hpp"
#include "core/observer.hpp"
/*#include "nvml.h"*/
#include "system/amd.hpp"
#include "system/file_utils.hpp"
#include "system/nvidia.hpp"
#include <algorithm>
#include <bits/chrono.h>
#include <cmath>
#include <cstdio>
#include <dirent.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <sys/stat.h>
#include <regex>

static auto findInput(const std::string& path, const char* input_prefix, std::string& input, const std::string& name) -> bool
{
    auto files = ls(path.c_str(), input_prefix, LS_FILES);
    for (auto& file : files) {
        if (std::string(file).ends_with("_label")) {
            continue;
}

        auto label = readLine(path + "/" + file);
        if (label != name) {
            continue;
}

        auto uscore = file.find_first_of("_");
        if (uscore != std::string::npos) {
            file.erase(uscore, std::string::npos);
            input = path + "/" + file + "_input";
            //9 characters should not overflow the 32-bit int
            return std::stoi(readLine(input).substr(0, 9)) > 0;
        }
    }
    return false;
}

static auto findFallbackInput(const std::string& path, const char* input_prefix, std::string& input) -> bool
{
    auto files = ls(path.c_str(), input_prefix, LS_FILES);
    if (files.empty()) {
        return false;
}

    std::sort(files.begin(), files.end());
    for (auto& file : files) {
        if (!std::string(file).ends_with("_input")) {
            continue;
}
        input = path + "/" + file;
        core::Logger::log_(core::LogLevel::INFO) << std::format("fallback cpu {} input: {}", input_prefix, input) << '\n';
        return true;
    }
    return false;
}

namespace sys {
    auto Monitoring::getCpuFile() -> bool {
        if (cpu_file != nullptr) {
            return true;
        }

        std::string name;
        std::string path;
        std::string input;
        std::string const hwmon = "/sys/class/hwmon/";

        auto dirs = ::ls(hwmon.c_str());
        for (auto& dir : dirs) {
            path = hwmon + dir;
            name = readLine(path + "/name");
            core::Logger::log_(core::LogLevel::INFO) << std::format("hwmon: sensor name: {}", name) << '\n';

            if (name == "coretemp") {
                findInput(path, "temp", input, "Package id 0");
                break;
            }
            if ((name == "zenpower" || name == "k10temp")) {
                if (!findInput(path, "temp", input, "Tdie"))
                    findInput(path, "temp", input, "Tctl");
                break;
            } else if (name == "atk0110") {
                findInput(path, "temp", input, "CPU Temperature");
                break;
            } else if (name == "it8603") {
                findInput(path, "temp", input, "temp1");
                break;
            } else if (std::string(name).starts_with("nct")) {
                // Only break if nct module has TSI0_TEMP node
                if (findInput(path, "temp", input, "TSI0_TEMP"))
                    break;

            } else if (name == "asusec") {
                // Only break if module has CPU node
                if (findInput(path, "temp", input, "CPU"))
                    break;
            } else {
                path.clear();
            }
        }
        if (path.empty() || (!fileExists(input) && !findFallbackInput(path, "temp", input))) {
            core::Logger::log_(core::LogLevel::WARNING) << std::format("Could not find cpu temp sensor location") << '\n';
            return false;
        }
        core::Logger::log_(core::LogLevel::INFO) << std::format("hwmon: using input: {}", input) << std::endl;
        cpu_file = fopen(input.c_str(), "r");

        return true;
    }

    void Monitoring::cpuInfoCpuName() {
        static const std::regex re(R"(\s+[0-9]+-Core Processor\s+)");
        uint32_t regs[4];
        for(int i=0x80000002; i<0x80000005; ++i) {
            __asm__ volatile (
                    "cpuid"
                    : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
                    : "a"(i)
                    );
            cpu_name += std::string(reinterpret_cast<char*>(regs), 16);
        }
        cpu_name = std::regex_replace(cpu_name, re, "");
    }

    auto Monitoring::readCpuTempFile(int &temp) -> bool {
        if (cpu_file == nullptr) {
            return false;
        }

        rewind(cpu_file);
        fflush(cpu_file);
        bool const ret = (fscanf(cpu_file, "%d", &temp) == 1);
        temp = temp / 1000;

        return ret;
    }

    auto Monitoring::getGpuName() -> std::string {
        return gpu->getGPUName();
    }

    auto Monitoring::getCpuName() -> std::string {
        return cpu_name;
    }

    Monitoring::Monitoring() {
        bool ret = false;

        getCpuFile();
        cpuInfoCpuName();

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
                                core::Logger::log_(core::LogLevel::ERROR) << entry.path().filename().string()
                                    << " -> Intel\n";
                                throw std::runtime_error("Intel GPU not supported");
                            } else {
                                std::cout << entry.path().filename().string()
                                    << " -> Unknown vendor: " << vendorId << "\n";
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            core::Logger::log_(core::LogLevel::INFO) << "Failed initialize GPU: " << e.what() << std::endl;

        }

        update();
        start();
    }

    Monitoring::~Monitoring(){
        if (cpu_file != nullptr) {
            fclose(cpu_file);
        }
        stop();
    }

    void Monitoring::start() {
        monitoring_thread = std::thread(&Monitoring::monitoringLoop, this);
    }

    void Monitoring::monitoringLoop() {
        using namespace std::chrono_literals;
        while (running.load()) {
            update();
            std::this_thread::sleep_for(1s);
        }
    }

    void Monitoring::stop() {
        running.store(false);
        if (monitoring_thread.joinable()) {
            monitoring_thread.join();
        }
    }

    void Monitoring::addObserver(const std::shared_ptr<core::Observer>& observer) {
        std::lock_guard<std::mutex> const lock(observer_lock);
        observers.push_back(observer);
    }

    void Monitoring::removeObserver(std::shared_ptr<core::Observer> observer) {
        std::erase_if(observers, [&observer](const std::shared_ptr<core::Observer>& o) {
            return o == observer;
        });

    }

    void Monitoring::notifyTempChanged(float temp, core::EventType event) {
        std::lock_guard<std::mutex> const lock(observer_lock);
        for(auto &&o : observers) {
            o->onEvent({event, temp});
        }
    }

    static auto to5 (size_t n) -> float {
        return std::round(static_cast<float>(n) / 5.0F) * 5.0F;
    }

    void Monitoring::fullUpdate() {
        full_update.store(true);
    }

    void Monitoring::update() {
        int temp = 0;
        unsigned int gtemp = 0;
        bool ret = readCpuTempFile(temp);
        ret = gpu->readGPUTemp(gtemp);
        if (full_update.load()) {
            notifyTempChanged(temp, core::EventType::CPU_TEMP_CHANGED);
            notifyTempChanged(gtemp, core::EventType::GPU_TEMP_CHANGED);
            full_update.store(false);
            return;
        }

        if (cpu_temp != temp) {
            notifyTempChanged(temp, core::EventType::CPU_TEMP_CHANGED);
        }
        if (gpu_temp != gtemp) {
            notifyTempChanged(gtemp, core::EventType::GPU_TEMP_CHANGED);
        }

        std::lock_guard<std::mutex> lock(temp_lock);
        cpu_temp = temp;
        gpu_temp = gtemp;
    }

    auto Monitoring::getCpuTemp() -> int {
        std::lock_guard<std::mutex> lock(temp_lock);
        return cpu_temp;
    }

    auto Monitoring::getGpuTemp() -> int {
        std::lock_guard<std::mutex> lock(temp_lock);
        return gpu_temp;
    }
}
