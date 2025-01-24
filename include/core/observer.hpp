#ifndef __OBSERVER_HPP__
#define __OBSERVER_HPP__

#include "core/fan_controller.hpp"
#include <memory>

namespace core {

    enum class EventType {
        CPU_TEMP_CHANGED,
        GPU_TEMP_CHANGED,
    };

    struct Event {
        EventType type;
        float value;
    };

    class Observer{
        public:
            virtual void onEvent(const Event& event) = 0;
    };

    class ObserverCPU : public Observer {
        public:
            ObserverCPU(std::shared_ptr<FanController> fc) :
                fan_controller(fc)
        {}
            void onEvent(const Event& event) override;
        private:
            std::shared_ptr<FanController> fan_controller;
    };

    class ObserverGPU : public Observer {
        public:
            ObserverGPU(std::shared_ptr<FanController> fc) :
                fan_controller(fc)
        {}
            void onEvent(const Event& event) override;
        private:
            std::shared_ptr<FanController> fan_controller;
    };
};
#endif // !__OBSERVER_HPP__
