#include "core/fan_controller.hpp"

#include <math.h>

#include <memory>
#include <mutex>
#include <ostream>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "core/observer.hpp"
#include "system/config.hpp"

namespace core {

void FanController::rgbThreadLoop() {
    while (run.load()) {
        hid_lock.lock();
        for (auto&& [i, c] : std::ranges::views::enumerate(color_buffer)) {
            for (auto&& [j, f] : std::ranges::views::enumerate(c)) {
                wrapper->setRGB(i, j + 1, color_buffer[i][j]);
            }
        }
        hid_lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}

void FanController::setMediator(std::shared_ptr<Mediator> mediator) {
    this->mediator = std::move(mediator);
}

void FanController::updateCPUfans(float temp) {
    updateFans(sys::MonitoringMode::MONITORING_CPU, temp);
}

void FanController::updateGPUfans(float temp) {
    updateFans(sys::MonitoringMode::MONITORING_GPU, temp);
}

void FanController::updateFanColor(std::size_t controller_idx,
                                   std::size_t fan_idx,
                                   std::array<float, 3> const& color,
                                   bool to_all) {
    std::lock_guard<std::mutex> lock(hid_lock);
    if (!to_all) {
        color_buffer[controller_idx][fan_idx] = color;
    } else {
        for(auto &&c : color_buffer) {
            std::for_each(c.begin(), c.end(), [&color](std::array<float, 3>& col){
                col = color;
            });
        }
    }
}

void FanController::updateFans(sys::MonitoringMode mode, float temp) {
    Logger::log(LogLevel::INFO) << "Update fans: " << temp << std::endl;
    std::ostringstream log_str;
    std::lock_guard<std::mutex> lock(hid_lock);
    for (auto&& c : system->getControllers()) {
        for (auto&& f : c.getFans()) {
            if (f.getMonitoringMode() == mode) {
                double s = NAN;
                if (dataUse == DataUse::POINT) {
                    s = f.getData().getSpeedForTemp(temp);
                } else {
                    s = f.getBData().getSpeedForTemp(temp);
                }
                auto stats = wrapper->sentToFan(c.getIdx(), f.getIdx() + 1,
                                                static_cast<uint>(s));
                log_str << "Mode "
                        << (mode == sys::MonitoringMode::MONITORING_GPU)
                        << " Controller " << c.getIdx() << " Fan " << f.getIdx()
                        << " set speed " << s << " on temp " << temp << '\n';
                log_str << "Speed: " << stats.first << "RPM: " << stats.second
                          << std::endl;
                if (mediator) {
                    mediator->notify(
                        EventMessageType::UPDATE_STATS,
                        std::make_shared<StatsMessage>(
                            StatsMessage{c.getIdx(), f.getIdx(), stats.first,
                                         stats.second}));
                }
            }
        }
    }
    Logger::log(LogLevel::INFO) << log_str.str() << std::endl;
}

};  // namespace core
