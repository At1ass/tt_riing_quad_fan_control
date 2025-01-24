#include "core/fan_mediator.hpp"
#include "core/fan_controller.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "gui/ui.hpp"
#include <iostream>

namespace core {
    void FanMediator::dispatch(EventMessageType eventType, const Message& msg)  {
        switch (eventType) {
            case EventMessageType::Initialize:
                initialize();
                break;

            case EventMessageType::UpdateGraph:
                handleUpdateGraph(msg);
                break;

            case EventMessageType::UpdateFan:
                handleUpdateFan(msg);
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
                }
            }

            Logger::log_(LogLevel::INFO) << "Initial data loaded into GuiManager." << std::endl;
        }
    }

    void FanMediator::handleUpdateGraph(const Message& msg) {
        if (fanController) {
            fanController->updateFanData(msg.c_idx, msg.f_idx, msg.t, msg.s);
            Logger::log_(LogLevel::INFO) << "Fan data updated from GUI for controller " << msg.c_idx << " fan " << msg.f_idx << std::endl;
        }
    }

    void FanMediator::handleUpdateFan(const Message& msg) {
        if (guiManager) {
            guiManager->updateGraphData(msg.c_idx, msg.f_idx, msg.t, msg.s);
            Logger::log_(LogLevel::INFO) << "Graph data updated from FanController for controller" << msg.c_idx << " fan " << msg.f_idx << std::endl;
        }
    }
}
