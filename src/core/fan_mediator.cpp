#include "core/fan_mediator.hpp"
#include "core/fan_controller.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "gui/ui.hpp"
#include "system/config.hpp"
#include <iostream>
#include <memory>

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
                    guiManager->updateGraphData(c.getIdx(), f.getIdx(), *data.getTData(), *data.getSData());
                    guiManager->updateFanMonitoringMods(c.getIdx(), f.getIdx(), f.getMonitoringMode() == sys::MONITORING_MODE::MONITORING_CPU ? 0 : 1);
                }
            }

            Logger::log_(LogLevel::INFO) << "Initial data loaded into GuiManager." << std::endl;
        }
    }

    void FanMediator::handleUpdateGraph(std::shared_ptr<DataMessage> msg) {
        if (fanController) {
            fanController->updateFanData(msg->c_idx, msg->f_idx, msg->t, msg->s);
            Logger::log_(LogLevel::INFO) << "Fan data updated from GUI for controller " << msg->c_idx << " fan " << msg->f_idx << std::endl;
        }
    }

    void FanMediator::handleUpdateFan(std::shared_ptr<DataMessage> msg) {
        if (guiManager) {
            guiManager->updateGraphData(msg->c_idx, msg->f_idx, msg->t, msg->s);
            Logger::log_(LogLevel::INFO) << "Graph data updated from FanController for controller" << msg->c_idx << " fan " << msg->f_idx << std::endl;
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
