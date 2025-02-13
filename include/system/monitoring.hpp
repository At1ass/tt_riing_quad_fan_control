#ifndef __MONITORING_HPP__
#define __MONITORING_HPP__

#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "core/observer.hpp"
#include "system/CPUController.hpp"
#include "system/GPUController.hpp"

namespace sys {

class Monitoring {
   public:
    Monitoring(Monitoring&&) = delete;
    Monitoring& operator=(Monitoring const&) = delete;
    Monitoring& operator=(Monitoring&&) = delete;
    Monitoring(std::unique_ptr<ICPUController> cpu_p,
               std::unique_ptr<IGPUController> gpu_p,
               std::chrono::milliseconds interval = std::chrono::seconds(1))
        : cpu(std::move(cpu_p)), gpu(std::move(gpu_p)), interval(interval) {
        update();
        start();
    }

    Monitoring(Monitoring const& m) = delete;
    ~Monitoring();

    void addObserver(std::shared_ptr<core::Observer> const& observer);
    void removeObserver(std::shared_ptr<core::Observer> observer);
    void notifyTempChanged(float temp, core::EventType event);
    std::string getGpuName();
    std::string getCpuName();

   private:
    void start();
    void stop();
    void monitoringLoop();
    void update();

    int cpu_temp{};
    int gpu_temp{};
    std::string cpu_name;
    std::atomic<bool> running = true;
    std::thread monitoring_thread;
    std::mutex observer_lock;
    std::chrono::milliseconds interval = std::chrono::seconds(1);
    std::vector<std::shared_ptr<core::Observer>> observers;
    std::unique_ptr<IGPUController> gpu;
    std::unique_ptr<ICPUController> cpu;
};

}  // namespace sys
#endif  // !__MONITORING_HPP__
