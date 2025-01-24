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
namespace core {
    class FanMediator : public Mediator {
        public:
            FanMediator(std::shared_ptr<gui::GuiManager> guiManager, std::shared_ptr<core::FanController> fanController)
                : guiManager(std::move(guiManager)), fanController(std::move(fanController)) {}

            void dispatch(EventMessageType eventType, const Message& msg);

        private:
            void initialize();

            void handleUpdateGraph(const Message& msg);

            void handleUpdateFan(const Message& msg);

            std::shared_ptr<gui::GuiManager> guiManager;
            std::shared_ptr<core::FanController> fanController;
    };
};
#endif //!__FAN_MEDIATOR_HPP__
