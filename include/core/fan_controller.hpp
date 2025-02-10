#ifndef __FAN_CONTROLLER_HPP__
#define __FAN_CONTROLLER_HPP__

#include "system/config.hpp"
#include "core/fan_mediator.hpp"
#include "system/hidapi_wrapper.hpp"
#include "core/mediator.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace core {
    enum class DATA_USE {
        POINT,
        BEZIER
    };

    class FanController {
        public:
            FanController(const FanController &) = default;
            FanController(FanController &&) = default;
            FanController &operator=(const FanController &) = default;
            FanController &operator=(FanController &&) = default;
            FanController(std::shared_ptr<sys::System> sys,
                    std::shared_ptr<sys::IHidWrapper> wr)
                : system(sys), wrapper(wr) {}

            void setMediator(std::shared_ptr<Mediator> mediator);
            void updateCPUfans(float temp);
            void updateGPUfans(float temp);
            void reloadAllFans();
            void updateFanData(int controller_idx, int fan_idx, const std::vector<double>& temperatures, const std::vector<double>& speeds);
            void updateFanData(int controller_idx, int fan_idx, const std::array<std::pair<double, double>, 4>& bdata);
            void updateFanMonitoringMode(int controller_idx, int fan_idx, const int& mode);
            void pointInfo() {
                dataUse = DATA_USE::POINT;
            }
            void bezierInfo() {
                dataUse = DATA_USE::BEZIER;
            }

            std::vector<sys::Controller>& getAllFanData() {
                return system->getControllers();
            }

        private:

            float to5 (size_t n) {
                return round((float)n / 5.0f) * 5.0f;
            }
            void updateFans(sys::MONITORING_MODE mode, float temp);

            DATA_USE dataUse = DATA_USE::POINT;
            std::shared_ptr<sys::System> system;
            std::shared_ptr<sys::IHidWrapper> wrapper;
            std::shared_ptr<Mediator> mediator;
    };

};
#endif // !__FAN_CONTROLLER_HPP__
