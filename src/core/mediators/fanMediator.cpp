#include <cstdint>
#include <iostream>
#include <memory>

#include "core/mediators/fanMediator.hpp"
#include "core/fanController.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "gui/ui.hpp"

constexpr uint8_t const COLOR_MULTIPLIER = 0xFF;

namespace core {

uint8_t FanMediator::convertChannel(float val) {
    return static_cast<unsigned char>(val * COLOR_MULTIPLIER);
}
void FanMediator::dispatch(EventMessageType event_type,
                           std::shared_ptr<Message> msg) {
    switch (event_type) {
        case EventMessageType::UPDATE_STATS:
            handleUpdateStats(std::static_pointer_cast<StatsMessage>(msg));
            break;

        case EventMessageType::UPDATE_COLOR:
            handleUpdateColor(std::static_pointer_cast<ColorMessage>(msg));
            break;

        case EventMessageType::UPDATE_EFFECT:
            handleUpdateEffect(std::static_pointer_cast<ColorMessage>(msg));
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
            std::array<uint8_t, 3>{convertChannel(msg->g),
                                   convertChannel(msg->r),
                                   convertChannel(msg->b)},
            msg->to_all);

        Logger::log(LogLevel::INFO)
            << "Color updated from FanController for "
               "controller"
            << msg->c_idx << " fan " << msg->f_idx << "Colors:" << msg->g << " "
            << msg->r << " " << msg->b << std::endl;
    }
}

void FanMediator::handleUpdateEffect(std::shared_ptr<ColorMessage> msg) {
    if (fanController) {
        fanController->updateEffect(
            msg->c_idx, msg->f_idx,
            std::array<uint8_t, 3>{convertChannel(msg->g),
                                   convertChannel(msg->r),
                                   convertChannel(msg->b)});
    }
}

}  // namespace core
