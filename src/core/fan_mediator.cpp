#include "core/fan_mediator.hpp"

#include <iostream>
#include <memory>
#include <variant>

#include "core/fan_controller.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "gui/ui.hpp"
#include "system/config.hpp"

namespace core {

void FanMediator::dispatch(EventMessageType event_type,
                           std::shared_ptr<Message> msg) {
    switch (event_type) {
        case EventMessageType::INITIALIZE:
            initialize();
            break;

        case EventMessageType::UPDATE_GRAPH:
            handleUpdateGraph(std::static_pointer_cast<DataMessage>(msg));
            break;

        case EventMessageType::UPDATE_FAN:
            handleUpdateFan(std::static_pointer_cast<DataMessage>(msg));
            break;

        case EventMessageType::UPDATE_MONITORING_MODE_UI:
            handleUpdateMonitoringModeUi(
                std::static_pointer_cast<ModeMessage>(msg));
            break;

        case EventMessageType::UPDATE_MONITORING_MODE_FAN:
            handleUpdateMonitoringModeFan(
                std::static_pointer_cast<ModeMessage>(msg));
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
            for (auto&& f : c.getFans()) {
                auto data = f.getData();
                guiManager->updateGraphData(
                    c.getIdx(), f.getIdx(),
                    FanData{*data.getTData(), *data.getSData()});
                auto bdata = f.getBData();
                guiManager->updateGraphData(c.getIdx(), f.getIdx(),
                                            bdata.getData());
                guiManager->updateFanMonitoringMods(
                    c.getIdx(), f.getIdx(),
                    f.getMonitoringMode() == sys::MonitoringMode::MONITORING_CPU
                        ? 0
                        : 1);
            }
        }

        Logger::log(LogLevel::INFO)
            << "Initial data loaded into GuiManager." << std::endl;
    }
}

void FanMediator::handleUpdateGraph(std::shared_ptr<DataMessage> msg) {
    if (fanController) {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, FanData>) {
                    auto data = std::get<FanData>(msg->data);
                    fanController->updateFanData(msg->c_idx, msg->f_idx, data.t,
                                                 data.s);
                    Logger::log(LogLevel::INFO)
                        << "Fan data updated from GUI for controller "
                        << msg->c_idx << " fan " << msg->f_idx << std::endl;
                } else if constexpr (std::is_same_v<
                                         T,
                                         std::array<std::pair<double, double>,
                                                    4>>) {
                    auto data =
                        std::get<std::array<std::pair<double, double>, 4>>(
                            msg->data);
                    fanController->updateFanData(msg->c_idx, msg->f_idx, data);
                    Logger::log(LogLevel::INFO)
                        << "Fan data updated from GUI for controller "
                        << msg->c_idx << " fan " << msg->f_idx << std::endl;
                }
            },
            msg->data);
    }
}

void FanMediator::handleUpdateFan(std::shared_ptr<DataMessage> msg) {
    if (guiManager) {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, FanData>) {
                    auto data = std::get<FanData>(msg->data);
                    guiManager->updateGraphData(msg->c_idx, msg->f_idx,
                                                FanData{data.t, data.s});
                    Logger::log(LogLevel::INFO)
                        << "Graph data updated from FanController for "
                           "controller"
                        << msg->c_idx << " fan " << msg->f_idx << std::endl;
                } else if constexpr (std::is_same_v<
                                         T,
                                         std::array<std::pair<double, double>,
                                                    4>>) {
                    auto data =
                        std::get<std::array<std::pair<double, double>, 4>>(
                            msg->data);
                    guiManager->updateGraphData(msg->c_idx, msg->f_idx, data);
                    Logger::log(LogLevel::INFO)
                        << "Graph data updated from FanController for "
                           "controller "
                        << msg->c_idx << " fan " << msg->f_idx << std::endl;
                }
            },
            msg->data);
    }
}

void FanMediator::handleUpdateMonitoringModeUi(
    std::shared_ptr<ModeMessage> msg) {
    if (fanController) {
        fanController->updateFanMonitoringMode(msg->c_idx, msg->f_idx,
                                               msg->mode);
        Logger::log(LogLevel::INFO)
            << "Fan monitoring mode updated to " << msg->mode
            << "from GUI for controller " << msg->c_idx << " fan " << msg->f_idx
            << std::endl;
    }
}

void FanMediator::handleUpdateMonitoringModeFan(
    std::shared_ptr<ModeMessage> msg) {
    if (guiManager) {
        guiManager->updateFanMonitoringMods(msg->c_idx, msg->f_idx, msg->mode);
        Logger::log(LogLevel::INFO)
            << "Monitoring source updated to " << msg->mode
            << "from FanController for controller" << msg->c_idx << " fan "
            << msg->f_idx << std::endl;
    }
}

}  // namespace core
