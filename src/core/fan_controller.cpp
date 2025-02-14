#include "core/fan_controller.hpp"

#include <math.h>

#include <memory>
#include <mutex>
#include <ostream>
#include <thread>
#include <utility>
#include <vector>

#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "system/config.hpp"

namespace core {

void FanController::rgbThreadLoop() {
    while (run.load()) {
        hid_lock.lock();
        for ( size_t c = 0; c < 4; c++) {
            for ( size_t f = 1; f <= 5; f++) {
                wrapper->setRGB(c, f);
            }
        }
        hid_lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
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

void FanController::reloadAllFans() {
    auto controllers = system->getControllers();
    for (auto&& c : controllers) {
        for (auto&& f : c.getFans()) {
            auto fd = f.getData().getData();
            updateFanData(c.getIdx(), f.getIdx(), fd.first, fd.second);
            updateFanMonitoringMode(
                c.getIdx(), f.getIdx(),
                f.getMonitoringMode() == sys::MonitoringMode::MONITORING_CPU
                    ? 0
                    : 1);
        }
    }
}

void FanController::updateFanData(std::size_t controller_idx,
                                  std::size_t fan_idx,
                                  std::vector<double> const& temperatures,
                                  std::vector<double> const& speeds) {
    system->getControllers()[controller_idx]
        .getFans()[fan_idx]
        .getData()
        .updateData(temperatures, speeds);

    if (mediator) {
        mediator->notify(
            EventMessageType::UPDATE_FAN,
            std::make_shared<DataMessage>(DataMessage{
                controller_idx, fan_idx, FanData{temperatures, speeds}}));
    }
}

void FanController::updateFanData(
    std::size_t controller_idx, std::size_t fan_idx,
    std::array<std::pair<double, double>, 4> const& bdata) {
    system->getControllers()[controller_idx]
        .getFans()[fan_idx]
        .getBData()
        .setData(bdata);

    if (mediator) {
        mediator->notify(EventMessageType::UPDATE_FAN,
                         std::make_shared<DataMessage>(
                             DataMessage{controller_idx, fan_idx, bdata}));
    }
}

void FanController::updateFanMonitoringMode(std::size_t controller_idx,
                                            std::size_t fan_idx,
                                            int const& mode) {
    system->getControllers()[controller_idx]
        .getFans()[fan_idx]
        .setMonitoringMode(mode == 0 ? sys::MonitoringMode::MONITORING_CPU
                                     : sys::MonitoringMode::MONITORING_GPU);

    if (mediator) {
        mediator->notify(EventMessageType::UPDATE_MONITORING_MODE_FAN,
                         std::make_shared<ModeMessage>(
                             ModeMessage{controller_idx, fan_idx, mode}));
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
                wrapper->sentToFan(c.getIdx(), f.getIdx() + 1,
                                   static_cast<uint>(s));
                log_str << "Mode "
                        << (mode == sys::MonitoringMode::MONITORING_GPU)
                        << " Controller " << c.getIdx() << " Fan " << f.getIdx()
                        << " set speed " << s << " on temp " << temp << '\n';
                std::cout << "Mode "
                        << (mode == sys::MonitoringMode::MONITORING_GPU)
                        << " Controller " << c.getIdx() << " Fan " << f.getIdx()
                        << " set speed " << s << " on temp " << temp << '\n';
            }
        }
    }
    Logger::log(LogLevel::INFO) << log_str.str() << std::endl;
}

};  // namespace core
