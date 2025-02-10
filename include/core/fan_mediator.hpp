#ifndef __FAN_MEDIATOR_HPP__
#define __FAN_MEDIATOR_HPP__

#include "core/mediator.hpp"
#include <memory>
#include <utility>

namespace gui {
    class GuiManager;
}

namespace core {
    class FanController;
    class Monitoring;
}

    struct fanData{
        std::vector<double> t;
        std::vector<double> s;
    };

    struct DataMessage : public Message {
        std::variant<fanData, std::array<std::pair<double, double>, 4>> data;
    };

    struct ModeMessage : public Message {
        int mode;
    };

namespace core {
    class FanMediator : public Mediator {
        public:
            FanMediator(std::shared_ptr<gui::GuiManager> guiManager, std::shared_ptr<core::FanController> fanController)
                : guiManager(std::move(guiManager)), fanController(std::move(fanController)) {}

            void notify(EventMessageType eventType, std::shared_ptr<Message> msg) {
                dispatch(eventType, msg);
            }

        private:
            void dispatch(EventMessageType eventType, std::shared_ptr<Message> msg);

            void initialize();

            void handleUpdateGraph(std::shared_ptr<DataMessage> msg);

            void handleUpdateFan(std::shared_ptr<DataMessage> msg);

            void handleUpdateMonitoringModeUi(std::shared_ptr<ModeMessage> msg);

            void handleUpdateMonitoringModeFan(std::shared_ptr<ModeMessage> msg);

            std::shared_ptr<gui::GuiManager> guiManager;
            std::shared_ptr<core::FanController> fanController;
    };
};
#endif //!__FAN_MEDIATOR_HPP__
