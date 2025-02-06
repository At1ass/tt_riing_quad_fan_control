#include "system/amd.hpp"
#include "core/logger.hpp"
#include <filesystem>
#include <iterator>
#include <stdexcept>

namespace sys {
    void AMD::findGPUTempFile(const std::string& card) {
        namespace fs = std::filesystem;

        fs::path hwmon_dir = "/sys/class/drm/" + card + "/device/hwmon";

        if (!fs::exists(hwmon_dir)) {
            core::Logger::log_(core::LogLevel::ERROR) << hwmon_dir << " not exists";
            throw std::exception();
        }

        for (const auto &entry : fs::directory_iterator(hwmon_dir)) {
            if (entry.is_directory()) {
                auto nameFile = entry.path() / "name";
                if (fs::exists(nameFile)) {
                    std::ifstream fin(nameFile);
                    std::string name;
                    std::getline(fin, name);
                    if (name == "amdgpu") {
                        core::Logger::log_(core::LogLevel::INFO) << "Found AMD GPU hwmon: "
                            << entry.path().filename().string() << std::endl;
                        gpu_temp_file.open(entry.path() / "temp1_input");

                        if (!gpu_temp_file.is_open()) {
                            throw std::runtime_error("Сannot open gpu temp file");
                        }
                    }
                }
            }
        }
    }

    bool AMD::calculateGPUName() {
        std::string cmd = "lspci -vmm -d 1002: | grep ^Device: | head -n1 | sed \"s/Device:\\s//\"";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            core::Logger::log_(core::LogLevel::ERROR) << "Failed to run: " << cmd << "\n";
            return false;
        }

        // Читаем построчно
        std::array<char, 512> buffer;
        bool found = false;
        while (fgets(buffer.data(), buffer.size(), pipe)) {
            std::string line(buffer.data());
            // Просто выведем всё, что нашли
            found = true;
            core::Logger::log_(core::LogLevel::INFO) << "AMD GPU info: " << line;
            gpu_name = line;
        }
        pclose(pipe);

        if (!found) {
            core::Logger::log_(core::LogLevel::WARNING) << "No AMD GPU device found (or no lspci output).\n";
        }

        return true;
    }

    AMD::AMD(const std::string& card) {
        findGPUTempFile(card);

        if (!calculateGPUName()) {
            throw  std::runtime_error("Failed to get AMD GPU name");
        }
    }

    AMD::~AMD() {
        if (gpu_temp_file.is_open()) {
            gpu_temp_file.close();
        }

    }

    auto AMD::getGPUName() -> std::string {
        return gpu_name;
    }

    auto AMD::getGPUTemp() -> float {
        return gpu_temp;
    }

    auto AMD::readGPUTemp(unsigned int& temp) -> bool {
        gpu_temp_file.clear();
        gpu_temp_file.seekg(0, std::ios::beg);

        int gtemp;
        gpu_temp_file >> gtemp;

        if (gpu_temp_file.fail()) {
            core::Logger::log_(core::LogLevel::INFO) << "Cannot read gpu temp" << std::endl;
        }

        temp = static_cast<unsigned int>(gtemp);
        gpu_temp = temp;
        return true;
    }

}
