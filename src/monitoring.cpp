#include "file_utils.hpp"
#include "monitoring.hpp"
#include "nvml.h"
#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <dirent.h>
#include <vector>
#include <sys/stat.h>

static bool find_input(const std::string& path, const char* input_prefix, std::string& input, const std::string& name)
{
    auto files = ls(path.c_str(), input_prefix, LS_FILES);
    for (auto& file : files) {
        if (std::string(file).ends_with("_label"))
            continue;

        auto label = read_line(path + "/" + file);
        if (label != name)
            continue;

        auto uscore = file.find_first_of("_");
        if (uscore != std::string::npos) {
            file.erase(uscore, std::string::npos);
            input = path + "/" + file + "_input";
            //9 characters should not overflow the 32-bit int
            return std::stoi(read_line(input).substr(0, 9)) > 0;
        }
    }
    return false;
}

static bool find_fallback_input(const std::string& path, const char* input_prefix, std::string& input)
{
    auto files = ls(path.c_str(), input_prefix, LS_FILES);
    if (!files.size())
        return false;

    std::sort(files.begin(), files.end());
    for (auto& file : files) {
        if (!std::string(file).ends_with("_input"))
            continue;
        input = path + "/" + file;
        std::cerr << std::format("fallback cpu {} input: {}", input_prefix, input) << std::endl;
        return true;
    }
    return false;
}

bool monitoring::GetCpuFile() {
    if (cpu_file)
        return true;

    std::string name, path, input;
    std::string hwmon = "/sys/class/hwmon/";

    auto dirs = ::ls(hwmon.c_str());
    for (auto& dir : dirs) {
        path = hwmon + dir;
        name = read_line(path + "/name");
        std::cerr << std::format("hwmon: sensor name: {}", name) << std::endl;

        if (name == "coretemp") {
            find_input(path, "temp", input, "Package id 0");
            break;
        }
        else if ((name == "zenpower" || name == "k10temp")) {
            if (!find_input(path, "temp", input, "Tdie"))
                find_input(path, "temp", input, "Tctl");
            break;
        } else if (name == "atk0110") {
            find_input(path, "temp", input, "CPU Temperature");
            break;
        } else if (name == "it8603") {
            find_input(path, "temp", input, "temp1");
            break;
        } else if (std::string(name).starts_with("nct")) {
            // Only break if nct module has TSI0_TEMP node
            if (find_input(path, "temp", input, "TSI0_TEMP"))
                break;

        } else if (name == "asusec") {
            // Only break if module has CPU node
            if (find_input(path, "temp", input, "CPU"))
                break;
        } else {
            path.clear();
        }
    }
    if (path.empty() || (!file_exists(input) && !find_fallback_input(path, "temp", input))) {
        std::cerr << std::format("Could not find cpu temp sensor location") << std::endl;
        return false;
    } else {
        std::cerr << std::format("hwmon: using input: {}", input) << std::endl;
        cpu_file = fopen(input.c_str(), "r");
    }
    return true;
}

bool monitoring::read_cpu_temp_file(int &temp) {
	if (!cpu_file)
		return false;

	rewind(cpu_file);
	fflush(cpu_file);
	bool ret = (fscanf(cpu_file, "%d", &temp) == 1);
	temp = temp / 1000;

	return ret;
}

bool monitoring::init_nvml() {
    nvmlReturn_t result;
    char name[NVML_DEVICE_NAME_BUFFER_SIZE];
    int ret;

    result = nvmlInit();
    if (NVML_SUCCESS != ret) {
        std::cerr << "Failed to initialize NVML: " << nvmlErrorString(result) << std::endl;
        return false;
    }

    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (NVML_SUCCESS != result) {
        std::cerr << "Failed to get handle for device: " << nvmlErrorString(result) << std::endl;
        return false;
    }

    result = nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
    if (NVML_SUCCESS != result) {
        std::cerr << "Failed to get name of device:" << nvmlErrorString(result) << std::endl;
        return false;
    }

    gpu_name = std::string(name);

    return true;
}

bool monitoring::read_gpu_temp(unsigned int &temp) {
    nvmlReturn_t result;

    result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
    if (NVML_SUCCESS != result) {
        std::cerr << "Failed to get gpu temp: " << nvmlErrorString(result) << std::endl;
        return false;
    }

    return true;
}

std::string monitoring::get_gpu_name() {
    return gpu_name;
}

void monitoring::close_nvml() {
    nvmlShutdown();
}

monitoring::monitoring() {
    bool ret;

    GetCpuFile();
    ret = init_nvml();
    if (!ret) {
        throw std::runtime_error("Failed get NVIDIA device");
    }

    update();

}

monitoring::~monitoring(){
    if (cpu_file)
        fclose(cpu_file);
}

void monitoring::update() {
    int temp = 0;
    unsigned int gtemp = 0;
    bool ret = read_cpu_temp_file(temp);
    cpu_temp = temp;
    ret = read_gpu_temp(gtemp);
    gpu_temp = gtemp;
}

int monitoring::get_cpu_temp() {
    return cpu_temp;
}

int monitoring::get_gpu_temp() { return gpu_temp;}
