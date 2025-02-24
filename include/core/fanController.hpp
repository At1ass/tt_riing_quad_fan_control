#ifndef __FAN_CONTROLLER_HPP__
#define __FAN_CONTROLLER_HPP__

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include "core/effectsEngine.hpp"
#include "core/mediators/fanMediator.hpp"
#include "core/mediator.hpp"
#include "system/controllerData.hpp"
#include "system/deviceController.hpp"

constexpr std::chrono::milliseconds const DEFAULT_INTERVAL =
    std::chrono::milliseconds(100);

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
                  std::unique_ptr<EffectsEngine> ee, bool run = true,
                  std::chrono::milliseconds interval = DEFAULT_INTERVAL)
        : system(sys),
          wrapper(wr),
          effectsEngine(std::move(ee)),
          run(run),
          interval(interval) {
        color_buffer = wr->makeColorBuffer();
        tmp_color_buffer = wr->makeColorBuffer();
        rgb_thread = std::thread(&FanController::rgbThreadLoop, this);
        effects_thread = std::thread(&FanController::effectsThreadLoop, this);
    }
    ~FanController() {
        run.store(false);
        if (rgb_thread.joinable()) {
            rgb_thread.join();
        }
        if (effects_thread.joinable()) {
            effects_thread.join();
        }
    }

    void setMediator(std::shared_ptr<Mediator> mediator);
    void updateCPUfans(float temp);
    void updateGPUfans(float temp);
    void updateFanColor(std::size_t controller_idx, std::size_t fan_idx,
                        std::array<uint8_t, 3> const& color, bool to_all);
    void updateEffect(std::size_t effect_pos, std::size_t duration_s,
                      std::array<uint8_t, 3> const& color);
    void pointInfo() { dataUse = DataUse::POINT; }
    void bezierInfo() { dataUse = DataUse::BEZIER; }

    std::vector<sys::Controller>& getAllFanData() {
        return system->getControllers();
    }

   private:
    void rgbThreadLoop();
    void effectsThreadLoop();
    void updateFans(sys::MonitoringMode mode, float temp);

    DataUse dataUse = DataUse::POINT;
    std::vector<std::vector<std::array<uint8_t, 3>>> color_buffer;
    std::vector<std::vector<std::array<uint8_t, 3>>> tmp_color_buffer;
    std::shared_ptr<sys::System> system;
    std::shared_ptr<sys::DeviceController> wrapper;
    std::shared_ptr<Mediator> mediator;
    std::unique_ptr<EffectsEngine> effectsEngine;
    std::chrono::milliseconds interval;
    std::atomic<bool> run = true;
    std::thread rgb_thread;
    std::thread effects_thread;
    std::mutex hid_lock;
};

};  // namespace core
#endif  // !__FAN_CONTROLLER_HPP__
