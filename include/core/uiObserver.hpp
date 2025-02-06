#ifndef __UI_OBSERVER__
#define __UI_OBSERVER__

#include "core/observer.hpp"
#include "gui/ui.hpp"
#include <memory>

namespace core {
    class ObserverUiCPU : public Observer {
        public:
            ObserverUiCPU(std::shared_ptr<gui::GuiManager> gm) :
                gui_manager(gm)
        {}
            void onEvent(const Event& event) override;
        private:
            std::shared_ptr<gui::GuiManager> gui_manager;
    };

    class ObserverUiGPU : public Observer {
        public:
            ObserverUiGPU(std::shared_ptr<gui::GuiManager> gm) :
                gui_manager(gm)
        {}
            void onEvent(const Event& event) override;
        private:
            std::shared_ptr<gui::GuiManager> gui_manager;
    };
}
#endif // !__UI_OBSERVER__
