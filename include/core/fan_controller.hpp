#ifndef __FAN_CONTROLLER_HPP__
#define __FAN_CONTROLLER_HPP__

#include <memory>
#include <thread>
#include <vector>

#include "core/fan_mediator.hpp"
#include "core/mediator.hpp"
#include "system/config.hpp"
#include "system/hidapi_wrapper.hpp"

namespace core {

enum class DataUse { POINT, BEZIER };

class FanController {
   public:
    FanController(FanController const&) = delete;
    FanController(FanController&&) = delete;
    FanController& operator=(FanController const&) = delete;
    FanController& operator=(FanController&&) = delete;
    FanController(std::shared_ptr<sys::System> sys,
                  std::shared_ptr<sys::DeviceController> wr,
                  bool run = true)
        : system(sys), wrapper(wr), run(run) {
            rgb_thread = std::thread(&FanController::rgbThreadLoop, this);
        }
    ~FanController() {
        run.store(false);
        if (rgb_thread.joinable()) {
            rgb_thread.join();
        }
    }

    void setMediator(std::shared_ptr<Mediator> mediator);
    void updateCPUfans(float temp);
    void updateGPUfans(float temp);
    void reloadAllFans();
    void updateFanData(std::size_t controller_idx, std::size_t fan_idx,
                       std::vector<double> const& temperatures,
                       std::vector<double> const& speeds);
    void updateFanData(std::size_t controller_idx, std::size_t fan_idx,
                       std::array<std::pair<double, double>, 4> const& bdata);
    void updateFanMonitoringMode(std::size_t controller_idx,
                                 std::size_t fan_idx, int const& mode);
    void pointInfo() { dataUse = DataUse::POINT; }
    void bezierInfo() { dataUse = DataUse::BEZIER; }

    std::vector<sys::Controller>& getAllFanData() {
        return system->getControllers();
    }

   private:
    void rgbThreadLoop();
    void updateFans(sys::MonitoringMode mode, float temp);

    DataUse dataUse = DataUse::POINT;
    std::shared_ptr<sys::System> system;
    std::shared_ptr<sys::DeviceController> wrapper;
    std::shared_ptr<Mediator> mediator;
    std::atomic<bool> run = true;
    std::thread rgb_thread;
    std::mutex hid_lock;
};

};  // namespace core
#endif  // !__FAN_CONTROLLER_HPP__
