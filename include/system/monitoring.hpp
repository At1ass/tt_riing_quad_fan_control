#ifndef __MONITORING_HPP__
#define __MONITORING_HPP__

#include "nvml.h"
#include "core/observer.hpp"
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace sys {
    class Monitoring {
        public:
            Monitoring();
            Monitoring(const Monitoring &m) = delete;
            ~Monitoring();

            void addObserver(const std::shared_ptr<core::Observer>& observer);
            void removeObserver(std::shared_ptr<core::Observer> observer);
            void notifyTempChanged(float temp, core::EventType event);
            void fullUpdate();
            int getCpuTemp();
            std::string getGpuName();
            std::string getCpuName();
            int getGpuTemp();

        private:
            void cpuInfoCpuName();
            void start();
            void stop();
            void monitoringLoop();
            void update();
            bool getCpuFile();
            bool initNvml();
            static void closeNvml();
            bool readGpuTemp(unsigned int &temp);
            bool readCpuTempFile(int &temp);

            nvmlDevice_t device{};
            FILE *cpu_file = NULL;
            int cpu_temp{};
            int gpu_temp{};
            std::string gpu_name;
            std::string cpu_name;
            std::atomic<bool> running = true;
            std::atomic<bool> full_update = false;
            std::thread monitoring_thread;
            std::mutex observer_lock;
            std::mutex temp_lock;
            std::vector<std::shared_ptr<core::Observer>> observers;
    };
}
#endif // !__MONITORING_HPP__
