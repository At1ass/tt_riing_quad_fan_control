#include "system/monitoring.hpp"
#include "core/logger.hpp"
#include "core/observer.hpp"
#include "nvml.h"
#include "system/file_utils.hpp"
#include <algorithm>
#include <bits/chrono.h>
#include <cmath>
#include <cstdio>
#include <dirent.h>
#include <mutex>
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

    auto Monitoring::initNvml() -> bool {
        nvmlReturn_t result;
        char name[NVML_DEVICE_NAME_BUFFER_SIZE];
        int ret = 0;

        result = nvmlInit();
        if (NVML_SUCCESS != result) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to initialize NVML: " << nvmlErrorString(result) << '\n';
            return false;
        }

        result = nvmlDeviceGetHandleByIndex(0, &device);
        if (NVML_SUCCESS != result) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to get handle for device: " << nvmlErrorString(result) << '\n';
            return false;
        }

        result = nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
        if (NVML_SUCCESS != result) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to get name of device:" << nvmlErrorString(result) << '\n';
            return false;
        }

        gpu_name = std::string(name);

        return true;
    }

    auto Monitoring::readGpuTemp(unsigned int &temp) -> bool {
        nvmlReturn_t result;

        result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
        if (NVML_SUCCESS != result) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to get gpu temp: " << nvmlErrorString(result) << '\n';
            return false;
        }

        return true;
    }

    auto Monitoring::getGpuName() -> std::string {
        return gpu_name;
    }

    auto Monitoring::getCpuName() -> std::string {
        return cpu_name;
    }

    void Monitoring::closeNvml() {
        nvmlShutdown();
    }

    Monitoring::Monitoring() {
        bool ret = false;

        getCpuFile();
        cpuInfoCpuName();
        ret = initNvml();
        if (!ret) {
            throw std::runtime_error("Failed get NVIDIA device");
        }
        update();
        start();
    }

    Monitoring::~Monitoring(){
        if (cpu_file != nullptr) {
            fclose(cpu_file);
        }
        closeNvml();
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
        ret = readGpuTemp(gtemp);
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
