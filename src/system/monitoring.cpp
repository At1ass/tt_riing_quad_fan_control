#include "system/monitoring.hpp"

#include <bits/chrono.h>
#include <dirent.h>
#include <sys/stat.h>

#include <cmath>
#include <cstdio>
#include <memory>
#include <mutex>

#include "core/observer.hpp"

namespace sys {

auto Monitoring::getGpuName() -> std::string { return gpu->getGPUName(); }

auto Monitoring::getCpuName() -> std::string { return cpu->getCPUName(); }

Monitoring::~Monitoring() { stop(); }

void Monitoring::start() {
    monitoring_thread = std::thread(&Monitoring::monitoringLoop, this);
}

void Monitoring::monitoringLoop() {
    using namespace std::chrono_literals;
    while (running.load()) {
        update();
        std::this_thread::sleep_for(interval);
    }
}

void Monitoring::stop() {
    running.store(false);
    if (monitoring_thread.joinable()) {
        monitoring_thread.join();
    }
}

void Monitoring::addObserver(std::shared_ptr<core::Observer> const& observer) {
    std::lock_guard<std::mutex> const LOCK(observer_lock);
    observers.push_back(observer);
}

void Monitoring::removeObserver(std::shared_ptr<core::Observer> observer) {
    std::erase_if(observers,
                  [&observer](std::shared_ptr<core::Observer> const& o) {
                      return o == observer;
                  });
}

void Monitoring::notifyTempChanged(float temp, core::EventType event) {
    std::lock_guard<std::mutex> const LOCK(observer_lock);
    for (auto&& o : observers) {
        o->onEvent({event, temp});
    }
}

void Monitoring::update() {
    int temp = 0;
    unsigned int gtemp = 0;
    bool ret = cpu->readCpuTempFile(temp);
    ret = gpu->readGPUTemp(gtemp);

    notifyTempChanged(static_cast<float>(temp),
                      core::EventType::CPU_TEMP_CHANGED);
    notifyTempChanged(static_cast<float>(gtemp),
                      core::EventType::GPU_TEMP_CHANGED);

    cpu_temp = temp;
    gpu_temp = static_cast<int>(gtemp);
}

}  // namespace sys
