#include "system/CPUController.hpp"
#include "core/logger.hpp"
#include "system/file_utils.hpp"
#include "system/string_utils.hpp"
#include <regex>

namespace sys {

    constexpr int TEMP_DIVIDER = 1000;

    CPUController::CPUController() {
        getCPUFile();
        cpuInfoCpuName();
    }

    CPUController::~CPUController() {
        if (cpu_file.is_open())
            cpu_file.close();
    }

    auto CPUController::readCpuTempFile(int &temp) -> bool {
        if (!cpu_file.is_open()) {
            return false;
        }
        cpu_file.clear();
        cpu_file.seekg(0, std::ios::beg);

        int ctemp;
        cpu_file >> ctemp;

        if (cpu_file.fail()) {
            core::Logger::log_(core::LogLevel::INFO) << "Cannot read cpu temp" << std::endl;
            return false;
        }

        temp = static_cast<unsigned int>(ctemp / TEMP_DIVIDER);
        return true;
    }

    auto CPUController::getCPUName() -> std::string {
        return cpu_name;
    }

    bool CPUController::getCPUFile() {
        if (cpu_file.is_open()) {
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
        cpu_file.open(input.c_str());

        return true;
    }

    void CPUController::cpuInfoCpuName() {
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
}
