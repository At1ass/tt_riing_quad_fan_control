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
    void dispatch(EventMessageType event_type, std::shared_ptr<Message> msg);

    void initialize();

    void handleUpdateGraph(std::shared_ptr<DataMessage> msg);

    void handleUpdateFan(std::shared_ptr<DataMessage> msg);

    void handleUpdateMonitoringModeUi(std::shared_ptr<ModeMessage> msg);

    void handleUpdateMonitoringModeFan(std::shared_ptr<ModeMessage> msg);

    std::shared_ptr<gui::GuiManager> guiManager;
    std::shared_ptr<core::FanController> fanController;
};

};  // namespace core
#endif  //!__FAN_MEDIATOR_HPP__
