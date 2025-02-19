#ifndef __SYSTEM_BUILDER_HPP__
#define __SYSTEM_BUILDER_HPP__

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "core/logger.hpp"
#include "system/controllerData.hpp"
#include "toml.hpp"

constexpr int const DEFAULT_POINT_NUM = 21;
constexpr int const MAX_FAN = 5;

constexpr double const DEFAULT_SPEED = 50.0;
constexpr double const MIN_SPEED = 0.0;
constexpr double const MAX_SPEED = 100.0;
constexpr double const START_TEMP = 0.0;
constexpr double const STEP_TEMP = 5.0;
constexpr double const MIN_TEMP = 0.0;
constexpr double const MAX_TEMP = 100.0;
constexpr double const MIN_CP_POS = 0.0;
constexpr double const MAX_CP_POS = 100.0;
constexpr double const CONTROL_POINT_START_POSITION = 0.0;
constexpr double const CONTROL_POINT_MIDDLE_FIRST = 40.0;
constexpr double const CONTROL_POINT_MIDDLE_SECOND = 60.0;
constexpr double const CONTROL_POINT_END_POSITION = 100.0;

namespace sys {

// TODO(at1ass): Needed implemet color settings in config
class SystemBuilder {
   public:
    SystemBuilder() : system(std::make_shared<System>()) {};

    std::shared_ptr<System> buildFromFile(std::string_view path,
                                          std::size_t const CONTROLERS_NUM) {
        char const *home = nullptr;
        std::string default_path;
        if (path.empty()) {
            home = std::getenv("HOME");
            if (!home) {
                core::Logger::log(core::LogLevel::WARNING)
                    << "HOME enviroment variable not set" << std::endl;
            } else {
                default_path = std::string(home) + "/.config/config2.toml";
                path = default_path;
            }
        }

        auto config_data = toml::parse_file(path);

        if (auto* saved = config_data["saved"].as_array()) {
            std::size_t controller_idx = 0;
            for (auto const& controllers_node : *saved) {
                auto* controller_array = controllers_node.as_array();

                if (!controller_array) {
                    core::Logger::log(core::LogLevel::WARNING)
                        << "Controller data must be array of fans" << std::endl;
                    system->addController(
                        initDummyController(controller_idx++));
                    continue;
                }

                system->addController(
                    parseController(*controller_array, controller_idx++));
            }

        } else {
            core::Logger::log(core::LogLevel::WARNING)
                << "Cannot found \"saved\" in config file" << std::endl;
            *system = initDummySystem(CONTROLERS_NUM);
        }

        return system;
    }

   private:
    template <typename T>
    T getTomlValue(toml::table const& table, std::string const& key,
                   T default_value) {
        if (auto* node = table.get(key)) {
            return node->value<T>().value_or(default_value);
        }
        return default_value;
    }

    std::vector<double> initDummySpeeds() {
        std::vector<double> dummy_speeds;
        dummy_speeds.resize(DEFAULT_POINT_NUM);
        std::fill(dummy_speeds.begin(), dummy_speeds.end(), DEFAULT_SPEED);
        return dummy_speeds;
    }

    std::vector<double> initDummyTemps() {
        std::vector<double> dummy_temps;
        dummy_temps.resize(DEFAULT_POINT_NUM);
        std::generate(
            dummy_temps.begin(), dummy_temps.end(),
            [n = START_TEMP - STEP_TEMP, s = STEP_TEMP]() mutable { return n += s; });
        return dummy_temps;
    }

    std::array<std::pair<double, double>, 4> initDummyControlPoints() {
        return {std::make_pair(CONTROL_POINT_START_POSITION,
                               CONTROL_POINT_START_POSITION),
                std::make_pair(CONTROL_POINT_MIDDLE_SECOND,
                               CONTROL_POINT_MIDDLE_FIRST),
                std::make_pair(CONTROL_POINT_MIDDLE_FIRST,
                               CONTROL_POINT_MIDDLE_SECOND),
                std::make_pair(CONTROL_POINT_END_POSITION,
                               CONTROL_POINT_END_POSITION)};
    }

    FanSpeedData initDummyFanSpeedData() {
        FanSpeedData dummy_speed_data;

        dummy_speed_data.setSpeeds(initDummySpeeds());
        dummy_speed_data.setTemps(initDummyTemps());

        return dummy_speed_data;
    }

    FanBezierData initDummyFanBezierData() {
        FanBezierData dummy_bezier_data;

        dummy_bezier_data.setData(initDummyControlPoints());

        return dummy_bezier_data;
    }

    Fan initDummyFan(std::size_t const FAN_IDX) {
        Fan dummy_fan;

        dummy_fan.setIdx(FAN_IDX);
        dummy_fan.setMonitoringMode(MonitoringMode::MONITORING_CPU);
        dummy_fan.addData(initDummyFanSpeedData());
        dummy_fan.addBData(initDummyFanBezierData());

        return dummy_fan;
    }

    Controller initDummyController(std::size_t const CONTROLLER_IDX) {
        Controller dummy_controller;

        for (int i = 0; i < MAX_FAN; i++) {
            dummy_controller.addFan(initDummyFan(i));
        }

        return dummy_controller;
    }

    System initDummySystem(std::size_t const CONTROLLERS_NUM) {
        System dummy_system;

        for (int i = 0; i < CONTROLLERS_NUM; i++) {
            dummy_system.addController(initDummyController(i));
        }

        return dummy_system;
    }

    std::vector<double> parseSpeeds(toml::table const& fan_table) {
        std::vector<double> speeds;

        auto* speeds_node = fan_table.get("Speeds");
        if (!speeds_node || !speeds_node->as_array()) {
            core::Logger::log(core::LogLevel::WARNING)
                << "Speeds must be array." << std::endl;
            return initDummySpeeds();
        }

        for (auto const& speed_value : *speeds_node->as_array()) {
            if (!speed_value.is_floating_point()) {
                core::Logger::log(core::LogLevel::WARNING)
                    << "Speed must be a floating point" << std::endl;
                speeds.push_back(DEFAULT_SPEED);
                continue;
            }
            speeds.push_back(std::clamp(speed_value.as_floating_point()->get(),
                                        MIN_SPEED, MAX_SPEED));
        }

        return speeds;
    }

    std::vector<double> parseTemps(toml::table const& fan_table) {
        std::vector<double> temps;

        auto* temps_node = fan_table.get("Temps");
        if (!temps_node || !temps_node->as_array()) {
            core::Logger::log(core::LogLevel::WARNING)
                << "Speeds must be array." << std::endl;
            return initDummyTemps();
        }

        for (auto const& temp_value : *temps_node->as_array()) {
            if (!temp_value.is_floating_point()) {
                core::Logger::log(core::LogLevel::WARNING)
                    << "Temp must be a floating point" << std::endl;
                continue;
            }
            temps.push_back(std::clamp(temp_value.as_floating_point()->get(),
                                       MIN_TEMP, MAX_TEMP));
        }
        return temps;
    }

    std::array<std::pair<double, double>, 4> parseControlPoints(
        toml::table const& fan_table) {
        std::array<std::pair<double, double>, 4> control_points{};
        std::span<std::pair<double, double>> cp_span(control_points);
        int idx = 0;

        auto* cp_node = fan_table.get("Control points");
        if (!cp_node || !cp_node->as_array()) {
            core::Logger::log(core::LogLevel::WARNING)
                << "Control points must be array" << std::endl;
            return initDummyControlPoints();
        }
        for (auto const& cp : *cp_node->as_array()) {
            auto* cp_table = cp.as_table();
            if (!cp_table) {
                core::Logger::log(core::LogLevel::WARNING)
                    << "Incorrect point structure" << std::endl;
                return initDummyControlPoints();
            }
            double x = std::clamp(getTomlValue<double>(*cp_table, "x", 0),
                                  MIN_CP_POS, MAX_CP_POS);
            double y = std::clamp(getTomlValue<double>(*cp_table, "y", 0),
                                  MIN_CP_POS, MAX_CP_POS);
            cp_span[idx++] = std::move(std::pair<double, double>(x, y));

            if (idx >= 4) {
                break;
            }
        }

        if (idx != 4) {
            core::Logger::log(core::LogLevel::WARNING)
                << "Controll points number must be 4" << std::endl;
            return initDummyControlPoints();
        }

        return control_points;
    }

    FanSpeedData parseFanSpeedData(toml::table const& fan_table) {
        FanSpeedData speed_data;

        speed_data.setSpeeds(parseSpeeds(fan_table));
        speed_data.setTemps(parseTemps(fan_table));
        return speed_data;
    }

    FanBezierData parseFanBezierData(toml::table const& fan_table) {
        FanBezierData bezier_data;

        bezier_data.setData(parseControlPoints(fan_table));
        return bezier_data;
    }

    Fan parseFan(toml::table const& fan_table, std::size_t const FAN_IDX) {
        Fan fan;

        int mode = getTomlValue<int>(fan_table, "Monitoring", 0);
        if (mode != 0 && mode != 1) {
            core::Logger::log(core::LogLevel::WARNING)
                << "Incorrect monitoring mode " << mode << " for fan "
                << FAN_IDX << ". Monitoring mode must be 0(CPU) or 1(GPU)"
                << std::endl;
            mode = 0;
        }
        fan.setMonitoringMode(mode ? sys::MonitoringMode::MONITORING_GPU
                                   : sys::MonitoringMode::MONITORING_CPU);

        fan.setIdx(FAN_IDX);
        fan.addData(parseFanSpeedData(fan_table));
        fan.addBData(parseFanBezierData(fan_table));
        return fan;
    }

    Controller parseController(toml::array const& controller_array,
                               std::size_t const CONTROLLER_IDX) {
        Controller controller;
        controller.setIdx(CONTROLLER_IDX);
        std::size_t fan_idx = 0;

        for (auto const& fans_node : controller_array) {
            auto* fan_table = fans_node.as_table();

            if (fan_idx > MAX_FAN) {
                core::Logger::log(core::LogLevel::WARNING)
                    << "For one controller only 5 fans" << std::endl;
                break;
            }

            if (!fan_table) {
                core::Logger::log(core::LogLevel::WARNING)
                    << "Incorrect fan structure: expected table" << std::endl;
                controller.addFan(initDummyFan(fan_idx++));
                continue;
            }

            controller.addFan(parseFan(*fan_table, fan_idx++));
        }
        return controller;
    }

    std::shared_ptr<System> system;
};

}  // namespace sys

#endif  // !__SYSTEM_BUILDER_HPP__
