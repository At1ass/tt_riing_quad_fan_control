#ifndef __SYSTEM_BUILDER_HPP__
#define __SYSTEM_BUILDER_HPP__

#include <memory>
#include <string_view>
#include <vector>

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
                                          std::size_t const CONTROLERS_NUM);

   private:
    template <typename T>
    T getTomlValue(toml::table const& table, std::string const& key,
                   T default_value) {
        if (auto* node = table.get(key)) {
            return node->value<T>().value_or(default_value);
        }
        return default_value;
    }

    std::vector<double> initDummySpeeds();
    std::vector<double> initDummyTemps();
    std::array<std::pair<double, double>, 4> initDummyControlPoints();
    FanSpeedData initDummyFanSpeedData();
    FanBezierData initDummyFanBezierData();
    Fan initDummyFan(std::size_t const FAN_IDX);
    Controller initDummyController(std::size_t const CONTROLLER_IDX);
    System initDummySystem(std::size_t const CONTROLLERS_NUM);

    std::vector<double> parseSpeeds(toml::table const& fan_table);
    std::vector<double> parseTemps(toml::table const& fan_table);
    std::array<std::pair<double, double>, 4> parseControlPoints(
        toml::table const& fan_table);
    FanSpeedData parseFanSpeedData(toml::table const& fan_table);
    FanBezierData parseFanBezierData(toml::table const& fan_table);
    Fan parseFan(toml::table const& fan_table, std::size_t const FAN_IDX);
    Controller parseController(toml::array const& controller_array,
                               std::size_t const CONTROLLER_IDX);

    std::shared_ptr<System> system;
};

}  // namespace sys

#endif  // !__SYSTEM_BUILDER_HPP__
