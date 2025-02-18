#ifndef __FAN_CONTROLLER_HPP__
#define __FAN_CONTROLLER_HPP__

#include <array>
#include <chrono>
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
                  std::shared_ptr<sys::DeviceController> wr, bool run = true,
                  std::chrono::milliseconds interval = std::chrono::seconds(1))
        : system(sys), wrapper(wr), run(run), interval(interval) {
        color_buffer = wr->makeColorBuffer();
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
    void updateFanColor(std::size_t controller_idx, std::size_t fan_idx,
                       std::array<float, 3> const& color, bool to_all);
    void pointInfo() { dataUse = DataUse::POINT; }
    void bezierInfo() { dataUse = DataUse::BEZIER; }

    std::vector<sys::Controller>& getAllFanData() {
        return system->getControllers();
    }

   private:
    void rgbThreadLoop();
    void updateFans(sys::MonitoringMode mode, float temp);

    DataUse dataUse = DataUse::POINT;
    std::vector<std::vector<std::array<float, 3>>> color_buffer;
    std::shared_ptr<sys::System> system;
    std::shared_ptr<sys::DeviceController> wrapper;
    std::shared_ptr<Mediator> mediator;
    std::chrono::milliseconds interval;
    std::atomic<bool> run = true;
    std::thread rgb_thread;
    std::mutex hid_lock;
};

};  // namespace core
#endif  // !__FAN_CONTROLLER_HPP__
