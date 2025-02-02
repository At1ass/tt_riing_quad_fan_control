#include "core/fan_mediator.hpp"
#include "core/fan_controller.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "gui/ui.hpp"
#include "system/config.hpp"
#include <iostream>
#include <memory>
#include <variant>

namespace core {
    void FanMediator::dispatch(EventMessageType eventType, std::shared_ptr<Message> msg)  {
        switch (eventType) {
            case EventMessageType::Initialize:
                initialize();
                break;

            case EventMessageType::UpdateGraph:
                handleUpdateGraph(std::static_pointer_cast<DataMessage>(msg));
                break;

            case EventMessageType::UpdateFan:
                handleUpdateFan(std::static_pointer_cast<DataMessage>(msg));
                break;

            case EventMessageType::UpdateMonitoringModeUi:
                handleUpdateMonitoringModeUi(std::static_pointer_cast<ModeMessage>(msg));
                break;

            case EventMessageType::UpdateMonitoringModeFan:
                handleUpdateMonitoringModeFan(std::static_pointer_cast<ModeMessage>(msg));
                break;

            default:
                std::cerr << "Unhandled event type\n";
                break;
        }
    }

    void FanMediator::initialize() {
        if (fanController && guiManager) {
            auto controllers = fanController->getAllFanData();
            for (auto&& c : controllers) {
                for(auto&& f : c.getFans()) {
                    auto data = f.getData();
                    guiManager->updateGraphData(c.getIdx(), f.getIdx(), fanData{*data.getTData(), *data.getSData()});
                    auto bdata = f.getBData();
                    guiManager->updateGraphData(c.getIdx(), f.getIdx(), bdata.getData());
                    guiManager->updateFanMonitoringMods(c.getIdx(), f.getIdx(), f.getMonitoringMode() == sys::MONITORING_MODE::MONITORING_CPU ? 0 : 1);
                }
            }

            Logger::log_(LogLevel::INFO) << "Initial data loaded into GuiManager." << std::endl;
        }
    }

    void FanMediator::handleUpdateGraph(std::shared_ptr<DataMessage> msg) {
        if (fanController) {
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, fanData>) {
                    auto data = std::get<fanData>(msg->data);
                    fanController->updateFanData(msg->c_idx, msg->f_idx, data.t, data.s);
                    Logger::log_(LogLevel::INFO) << "Fan data updated from GUI for controller " << msg->c_idx << " fan " << msg->f_idx << std::endl;
                } else if constexpr (std::is_same_v<T, std::array<std::pair<double, double>, 4>>) {
                    auto data = std::get<std::array<std::pair<double, double>, 4>>(msg->data);
                    fanController->updateFanData(msg->c_idx, msg->f_idx, data);
                    Logger::log_(LogLevel::INFO) << "Fan data updated from GUI for controller " << msg->c_idx << " fan " << msg->f_idx << std::endl;
                }

            }, msg->data);
        }
    }

    void FanMediator::handleUpdateFan(std::shared_ptr<DataMessage> msg) {
        if (guiManager) {
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, fanData>) {
                    auto data = std::get<fanData>(msg->data);
                    guiManager->updateGraphData(msg->c_idx, msg->f_idx, fanData{data.t, data.s});
                    Logger::log_(LogLevel::INFO) << "Graph data updated from FanController for controller" << msg->c_idx << " fan " << msg->f_idx << std::endl;
                } else if constexpr (std::is_same_v<T, std::array<std::pair<double, double>, 4>>) {
                    auto data = std::get<std::array<std::pair<double, double>, 4>>(msg->data);
                    guiManager->updateGraphData(msg->c_idx, msg->f_idx, data);
                    Logger::log_(LogLevel::INFO) << "Graph data updated from FanController for controller " << msg->c_idx << " fan " << msg->f_idx << std::endl;
                }

            }, msg->data);
        }
    }

    void FanMediator::handleUpdateMonitoringModeUi(std::shared_ptr<ModeMessage> msg) {
        if (fanController) {
            fanController->updateFanMonitoringMode(msg->c_idx, msg->f_idx, msg->mode);
            Logger::log_(LogLevel::INFO) << "Fan monitoring mode updated to " << msg->mode << "from GUI for controller " << msg->c_idx << " fan " << msg->f_idx << std::endl;
        }
    }

    void FanMediator::handleUpdateMonitoringModeFan(std::shared_ptr<ModeMessage> msg) {
        if (guiManager) {
            guiManager->updateFanMonitoringMods(msg->c_idx, msg->f_idx, msg->mode);
            Logger::log_(LogLevel::INFO) << "Monitoring source updated to " << msg->mode << "from FanController for controller" << msg->c_idx << " fan " << msg->f_idx << std::endl;
        }
    }
}
