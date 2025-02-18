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
        case EventMessageType::UPDATE_STATS:
            handleUpdateStats(std::static_pointer_cast<StatsMessage>(msg));
            break;

        case EventMessageType::UPDATE_COLOR:
            handleUpdateColor(std::static_pointer_cast<ColorMessage>(msg));
            break;

        default:
            std::cerr << "Unhandled event type\n";
            break;
    }
}

void FanMediator::handleUpdateStats(std::shared_ptr<StatsMessage> msg) {
    if (guiManager) {
        guiManager->updateCurrentFanStats(msg->c_idx, msg->f_idx,
                                          msg->cur_speed, msg->cur_rpm);
        Logger::log(LogLevel::INFO) << "Stats updated from FanController for "
                                       "controller"
                                    << msg->c_idx << " fan " << msg->f_idx
                                    << " | Speed: " << msg->cur_speed
                                    << "RPM: " << msg->cur_rpm << std::endl;
    }
}

void FanMediator::handleUpdateColor(std::shared_ptr<ColorMessage> msg) {
    if (fanController) {
        fanController->updateFanColor(
            msg->c_idx, msg->f_idx,
            std::array<float, 3>{msg->g, msg->r, msg->b}, msg->to_all);

        Logger::log(LogLevel::INFO)
            << "Color updated from FanController for "
               "controller"
            << msg->c_idx << " fan " << msg->f_idx << "Colors:" << msg->g << " "
            << msg->r << " " << msg->b << std::endl;
    }
}

}  // namespace core
