#include "system/monitoring.hpp"
#include "core/observer.hpp"
#include <bits/chrono.h>
#include <cmath>
#include <cstdio>
#include <dirent.h>
#include <memory>
#include <mutex>
#include <sys/stat.h>

namespace sys {
    auto Monitoring::getGpuName() -> std::string {
        return gpu->getGPUName();
    }

    auto Monitoring::getCpuName() -> std::string {
        return cpu->getCPUName();
    }

    Monitoring::~Monitoring(){
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

    void Monitoring::update() {
        int temp = 0;
        unsigned int gtemp = 0;
        bool ret = cpu->readCpuTempFile(temp);
        ret = gpu->readGPUTemp(gtemp);

        notifyTempChanged(temp, core::EventType::CPU_TEMP_CHANGED);
        notifyTempChanged(gtemp, core::EventType::GPU_TEMP_CHANGED);

        cpu_temp = temp;
        gpu_temp = gtemp;
    }
}
