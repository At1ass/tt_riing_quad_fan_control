#include "core/fan_controller.hpp"
#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "system/config.hpp"
#include <memory>
#include <ostream>
#include <ranges>
#include <utility>
#include <vector>
namespace core {
    void FanController::setMediator(std::shared_ptr<FanMediator> mediator) {
        this->mediator = std::move(mediator);
    }

    void FanController::updateCPUfans(float temp) {
        updateFans(sys::MONITORING_MODE::MONITORING_CPU, temp);
    }

    void FanController::updateGPUfans(float temp) {
        updateFans(sys::MONITORING_MODE::MONITORING_GPU, temp);
    }

    void FanController::reloadAllFans() {
        auto controllers = system->getControllers();
        for(auto &&c : controllers) {
            for(auto &&f : c.getFans()) {
                auto fd = f.getData().getData();
                updateFanData(c.getIdx(), f.getIdx(), fd.first, fd.second);
                updateFanMonitoringMode(c.getIdx(), f.getIdx(), f.getMonitoringMode() == sys::MONITORING_MODE::MONITORING_CPU ? 0 : 1);
            }
        }
    }

    void FanController::updateFanData(int controller_idx, int fan_idx, const std::vector<double>& temperatures, const std::vector<double>& speeds) {
        system->getControllers()[controller_idx].getFans()[fan_idx].getData().updateData(temperatures, speeds);

        if(mediator) {
            mediator->notify(EventMessageType::UpdateFan, std::make_shared<DataMessage>(DataMessage{controller_idx, fan_idx, temperatures, speeds}));
        }
    }

    void FanController::updateFanMonitoringMode(int controller_idx, int fan_idx, const int& mode) {
        system->getControllers()[controller_idx].getFans()[fan_idx].setMonitoringMode(mode == 0 ? sys::MONITORING_MODE::MONITORING_CPU : sys::MONITORING_MODE::MONITORING_GPU);

        if(mediator) {
            mediator->notify(EventMessageType::UpdateMonitoringModeFan, std::make_shared<ModeMessage>(ModeMessage{controller_idx, fan_idx, mode}));
        }
    }

    void FanController::updateFans(sys::MONITORING_MODE mode, float temp) {
        Logger::log_(LogLevel::INFO) << "Update fans: " << temp << std::endl;
        std::ostringstream log_str;
        for(auto &&c : system->getControllers()) {
            for(auto &&f : c.getFans()) {
                if (f.getMonitoringMode() == mode) {
                    auto d = f.getData().getData();

                    for (auto &&[t, s] : std::views::zip(d.first, d.second)) {
                        if (to5(temp) == to5(t)) {
                            wrapper->sentToFan(c.getIdx(), f.getIdx() + 1, s);
                            log_str << "Mode " << (mode == sys::MONITORING_MODE::MONITORING_GPU) << " Controller " << c.getIdx() << " Fan " << f.getIdx() \
                                << " set speed " << s << " on temp " << temp << '\n';
                            break;
                        }
                    }
                }
            }
        }
        Logger::log_(LogLevel::INFO) << log_str.str() << std::endl;

    }
};
