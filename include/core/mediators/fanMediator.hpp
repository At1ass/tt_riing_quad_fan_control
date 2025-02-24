#ifndef __FAN_MEDIATOR_HPP__
#define __FAN_MEDIATOR_HPP__

#include <memory>
#include <utility>
#include <vector>

#include "core/mediator.hpp"

namespace gui {
class GuiManager;
}

namespace core {
class FanController;
class Monitoring;
}  // namespace core

struct FanData {
    std::vector<double> t;
    std::vector<double> s;
};

struct DataMessage : public Message {
    std::variant<FanData, std::array<std::pair<double, double>, 4>> data;
};

struct StatsMessage : public Message {
    std::size_t cur_speed;
    std::size_t cur_rpm;
};

struct ColorMessage : public Message {
    float r;
    float g;
    float b;
    bool to_all;
};

struct ModeMessage : public Message {
    int mode;
};

namespace core {
class FanMediator : public Mediator {
   public:
    FanMediator(std::shared_ptr<gui::GuiManager> gui_manager,
                std::shared_ptr<core::FanController> fan_controller)
        : guiManager(std::move(gui_manager)),
          fanController(std::move(fan_controller)) {}

    void notify(EventMessageType event_type,
                std::shared_ptr<Message> msg) override {
        dispatch(event_type, msg);
    }

   private:
    uint8_t convertChannel(float val);

    void dispatch(EventMessageType event_type, std::shared_ptr<Message> msg);

    void handleUpdateStats(std::shared_ptr<StatsMessage> msg);

    void handleUpdateColor(std::shared_ptr<ColorMessage> msg);

    void handleUpdateEffect(std::shared_ptr<ColorMessage> msg);

    std::shared_ptr<gui::GuiManager> guiManager;
    std::shared_ptr<core::FanController> fanController;
};

};  // namespace core
#endif  //!__FAN_MEDIATOR_HPP__
