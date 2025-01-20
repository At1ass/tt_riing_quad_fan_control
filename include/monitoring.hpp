#ifndef __MONITORING_HPP__
#define __MONITORING_HPP__

#include "nvml.h"
#include <cstdio>
#include <string>

class monitoring {
    public:
        monitoring();
        monitoring(const monitoring &m) = delete;
        ~monitoring();

        void update();
        int get_cpu_temp();
        std::string get_gpu_name();
        int get_gpu_temp();

    private:
        bool GetCpuFile();
        bool init_nvml();
        void close_nvml();
        bool read_gpu_temp(unsigned int &temp);
        bool read_cpu_temp_file(int &temp);

        nvmlDevice_t device;
        FILE *cpu_file;
        int cpu_temp;
        int gpu_temp;
        std::string gpu_name;
};

#endif // !__MONITORING_HPP__
