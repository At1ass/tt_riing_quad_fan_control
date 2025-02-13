#ifndef __UI_OBSERVER__
#define __UI_OBSERVER__

#include <memory>

#include "core/observer.hpp"
#include "gui/ui.hpp"

namespace core {
class ObserverUiCPU : public Observer {
   public:
    explicit ObserverUiCPU(std::shared_ptr<gui::GuiManager> gm)
        : gui_manager(gm) {}
    void onEvent(Event const& event) override;

   private:
    std::shared_ptr<gui::GuiManager> gui_manager;
};

class ObserverUiGPU : public Observer {
   public:
    explicit ObserverUiGPU(std::shared_ptr<gui::GuiManager> gm)
        : gui_manager(gm) {}
    void onEvent(Event const& event) override;

   private:
    std::shared_ptr<gui::GuiManager> gui_manager;
};
}  // namespace core
#endif  // !__UI_OBSERVER__
