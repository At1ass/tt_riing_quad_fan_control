#ifndef __OBSERVER_HPP__
#define __OBSERVER_HPP__

#include <memory>

#include "core/fan_controller.hpp"

namespace core {

enum class EventType {
    CPU_TEMP_CHANGED,
    GPU_TEMP_CHANGED,
};

struct Event {
    EventType type;
    float value;
};

class Observer {
   public:
    Observer(Observer const&) = default;
    Observer(Observer&&) = delete;
    Observer& operator=(Observer const&) = default;
    Observer& operator=(Observer&&) = delete;
    virtual ~Observer() = default;
    virtual void onEvent(Event const& event) = 0;

   protected:
    Observer() = default;
};

class ObserverCPU : public Observer {
   public:
    explicit ObserverCPU(std::shared_ptr<FanController> fc)
        : fan_controller(fc) {}
    void onEvent(Event const& event) override;

   private:
    std::shared_ptr<FanController> fan_controller;
};

class ObserverGPU : public Observer {
   public:
    explicit ObserverGPU(std::shared_ptr<FanController> fc)
        : fan_controller(fc) {}
    void onEvent(Event const& event) override;

   private:
    std::shared_ptr<FanController> fan_controller;
};

};  // namespace core
#endif  // !__OBSERVER_HPP__
