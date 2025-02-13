#include "system/config.hpp"

#include <math.h>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <span>
#include <sstream>
#include <stdexcept>

#include "core/logger.hpp"
#include "toml.hpp"

constexpr int const MIN_TEMP = 0;
constexpr int const MAX_TEMP = 100;
constexpr int const TEMP_STEP = 5;
constexpr float const DEFAULT_SPEED = 50.0;

constexpr double const BEGIN_POINT = 0.0;
constexpr double const END_POINT = 100.0;
constexpr double const DEFAULT_MIDDLE_FIRST_POINT = 40.0;
constexpr double const DEFAULT_MIDDLE_SECOND_POINT = 60.0;

constexpr double const EPSILON = 0.001;
constexpr std::size_t const MAX_ITERATIONS = 10000;

constexpr int const MAX_FAN = 5;

namespace sys {

FanSpeedData::FanSpeedData() {
    for (size_t k = MIN_TEMP; k <= MAX_TEMP; k += TEMP_STEP) {
        addTemp(static_cast<float>(k));
        addSpeed(DEFAULT_SPEED);
    }
}

void FanSpeedData::addSpeed(float s) {
    speeds.push_back(static_cast<double>(s));
}
void FanSpeedData::addTemp(float t) { temps.push_back(static_cast<double>(t)); }

void FanSpeedData::updateData(std::vector<double> t, std::vector<double> s) {
    temps.clear();
    speeds.clear();
    temps = std::move(t);
    speeds = std::move(s);
}

auto FanSpeedData::getTData() -> std::vector<double>* { return &temps; }
auto FanSpeedData::getSData() -> std::vector<double>* { return &speeds; }
auto FanSpeedData::getData()
    -> std::pair<std::vector<double>, std::vector<double>> {
    return {temps, speeds};
}

double FanSpeedData::getSpeedForTemp(float const& temp) {
    using pair = std::pair<double, double>;

    pair p1, p2;

    auto it = std::lower_bound(temps.begin(), temps.end(), temp);
    auto n = std::distance(temps.begin(), it);

    if (n == 0) return speeds[n];
    p1.first = temps[n];
    p2.first = temps[n - 1];
    p1.second = speeds[n];
    p2.second = speeds[n - 1];

    core::Logger::log(core::LogLevel::INFO)
        << "Points: " << "[" << p1.first << ", " << p1.second << "] ["
        << p2.first << ", " << p2.second << "]" << std::endl;

    return p1.second + ((p2.second - p1.second) / (p2.first - p1.first)) *
                           (temp - p1.first);
}

FanBezierData::FanBezierData() {
    setData({std::make_pair(BEGIN_POINT, BEGIN_POINT),
             std::make_pair(DEFAULT_MIDDLE_FIRST_POINT,
                            DEFAULT_MIDDLE_SECOND_POINT),
             std::make_pair(DEFAULT_MIDDLE_SECOND_POINT,
                            DEFAULT_MIDDLE_FIRST_POINT),
             std::make_pair(END_POINT, END_POINT)});
}

void FanBezierData::addControlPoint(std::pair<double, double> const& cp) {
    std::span<std::pair<double, double>> cp_span(controlPoints);
    cp_span[idx++] = std::move(cp);
}

auto FanBezierData::getData() -> std::array<std::pair<double, double>, 4>& {
    return controlPoints;
}

void FanBezierData::setData(
    std::array<std::pair<double, double>, 4> const& data) {
    controlPoints = std::move(data);
}

double FanBezierData::getSpeedForTemp(float const& temp) {
    using pair = std::pair<double, double>;
    double t_low = 0.0, t_high = 1.0;
    double t_mid = NAN;
    double const HALF = 2.0;

    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        t_mid = (t_low + t_high) / HALF;
        pair p = computeBezierAtT(t_mid);

        if (std::abs(p.first - temp) < EPSILON) {
            return p.second;  // Нашли нужное значение y
        }

        if (p.first < temp) {
            t_low = t_mid;
        } else {
            t_high = t_mid;
        }
    }

    return computeBezierAtT(t_mid)
        .second;  // Возвращаем ближайшее найденное значение
}

std::pair<double, double> FanBezierData::computeBezierAtT(double t) {
    double u = 1.0 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    std::pair<double, double> p;
    p.first =
        uuu * controlPoints[0].first + 3 * uu * t * controlPoints[1].first +
        3 * u * tt * controlPoints[2].first + ttt * controlPoints[3].first;
    p.second =
        uuu * controlPoints[0].second + 3 * uu * t * controlPoints[1].second +
        3 * u * tt * controlPoints[2].second + ttt * controlPoints[3].second;

    return p;
}

void Fan::addData(FanSpeedData const& data) { this->data = data; }

void Fan::addBData(FanBezierData const& bdata) { this->bdata = bdata; }

void Fan::setMonitoringMode(MonitoringMode const MODE) {
    monitoring_mode = MODE;
}

void Controller::addFan(Fan const& fan) { fans.push_back(fan); }

void System::addController(Controller const& controller) {
    controllers.push_back(controller);
}

auto Config::parseConfig(std::string_view path)
    -> std::shared_ptr<sys::System> {
    conf = toml::parse_file(path);
    auto* saved = conf["saved"].as_array();
    auto system = std::make_shared<sys::System>();
    if (saved != nullptr) {
        readed = true;
        size_t i = 0;
        size_t j = 0;
        for (auto&& d : *saved) {
            sys::Controller controller;
            controller.setIdx(i++);
            j = 0;
            d.as_array()->for_each([&controller, &j](auto& f) {
                if (toml::is_table<decltype(f)>) {
                    sys::Fan fan;
                    sys::FanSpeedData data;
                    sys::FanBezierData bdata;
                    data.resetData();
                    f.as_table()->for_each([&fan, &data, &bdata](
                                               toml::key const& key,
                                               auto&& val) {
                        if (toml::is_integer<decltype(val)> &&
                            key == "Monitoring") {
                            auto mode = val.as_integer()->get();
                            if (mode != 0 && mode != 1) {
                                throw std::runtime_error(
                                    "Incorrect monitoring mode");
                            }
                            fan.setMonitoringMode(
                                mode ? sys::MonitoringMode::MONITORING_GPU
                                     : sys::MonitoringMode::MONITORING_CPU);
                        } else if (toml::is_array<decltype(val)> &&
                                   key == "Temps") {
                            val.as_array()->for_each([&data](auto&& t) {
                                if (toml::is_floating_point<decltype(t)>) {
                                    data.addTemp(t.as_floating_point()->get());
                                } else {
                                    throw std::runtime_error("Incorrect temp");
                                }
                            });
                        } else if (toml::is_array<decltype(val)> &&
                                   key == "Speeds") {
                            val.as_array()->for_each([&data](auto&& s) {
                                if (toml::is_floating_point<decltype(s)>) {
                                    data.addSpeed(s.as_floating_point()->get());
                                } else {
                                    throw std::runtime_error("Incorrect speed");
                                }
                            });
                        } else if (toml::is_array<decltype(val)> &&
                                   key == "Control points") {
                            val.as_array()->for_each([&](auto& cp) {
                                if (toml::is_table<decltype(cp)>) {
                                    double x = NAN, y = NAN;
                                    std::pair<double, double> cpoint;
                                    cp.as_table()->for_each(
                                        [&](toml::key const& key,
                                            auto&& value) {
                                            if (toml::is_floating_point<
                                                    decltype(value)> &&
                                                key == "x") {
                                                cpoint.first =
                                                    value.as_floating_point()
                                                        ->get();
                                            } else if (toml::is_floating_point<
                                                           decltype(value)> &&
                                                       key == "y") {
                                                cpoint.second =
                                                    value.as_floating_point()
                                                        ->get();
                                            } else {
                                                throw std::runtime_error(
                                                    "Incorrect cp data");
                                            }
                                        });
                                    bdata.addControlPoint(cpoint);
                                } else {
                                    throw std::runtime_error(
                                        "Incorrect control points structure");
                                }
                            });
                            if (bdata.getIdx() != 4) {
                                throw std::runtime_error(
                                    "Control points count must be 4");
                            }
                        } else {
                            throw std::runtime_error(
                                "Incorrect config structure");
                        }
                    });
                    fan.setIdx(j++);
                    fan.addData(data);
                    fan.addBData(bdata);
                    controller.addFan(fan);
                } else {
                    throw std::runtime_error("Incorrect fan data");
                }
            });
            system->addController(controller);
        }
        return system;
    }
    initDummyFans(system);
    return system;
}

void Config::initDummyFans(std::shared_ptr<sys::System> const& system) const {
    for (size_t i = 0; i < controllers_num; i++) {
        sys::Controller controller;
        controller.setIdx(i);
        for (size_t j = 0; j < MAX_FAN; j++) {
            sys::Fan fan;
            fan.setIdx(j);
            fan.setMonitoringMode(sys::MonitoringMode::MONITORING_CPU);
            sys::FanSpeedData data;
            sys::FanBezierData bd;
            fan.addData(data);
            fan.addBData(bd);
            controller.addFan(fan);
        }
        system->addController(controller);
    }
}

void Config::printConfig(std::shared_ptr<sys::System> const& system) {
    std::ostringstream log_str;
    log_str << "Parsed config\n";
    for (auto&& c : system->getControllers()) {
        log_str << "Controller\n";
        for (auto&& f : c.getFans()) {
            log_str << "Fan (Monitoring "
                    << (f.getMonitoringMode() ==
                                sys::MonitoringMode::MONITORING_CPU
                            ? "CPU"
                            : "GPU")
                    << "): [";
            auto data = f.getData().getData();
            for (auto&& [t, s] : std::views::zip(data.first, data.second)) {
                log_str << "[ " << t << ", " << s << "], ";
            }
            log_str << "]\n";
        }
    }

    core::Logger::log(core::LogLevel::INFO) << log_str.str() << std::endl;
}

void Config::updateConf(std::shared_ptr<sys::System> const& system) {
    toml::array saved;

    for (auto&& c : system->getControllers()) {
        toml::array controller;
        for (auto&& f : c.getFans()) {
            auto data = f.getData().getData();
            auto bdata = f.getBData().getData();
            int mon_mode =
                f.getMonitoringMode() == sys::MonitoringMode::MONITORING_GPU
                    ? 1
                    : 0;
            toml::array temps;
            toml::array speeds;
            toml::array control_points;
            for (auto&& [t, s] : std::views::zip(data.first, data.second)) {
                temps.push_back(t);
                speeds.push_back(s);
            }
            for (auto&& cp : bdata) {
                toml::table pos{{"x", cp.first}, {"y", cp.second}};
                control_points.push_back(pos);
            }
            controller.insert(controller.cend(),
                              toml::table{{"Monitoring", mon_mode},
                                          {"Temps", temps},
                                          {"Speeds", speeds},
                                          {"Control points", control_points}});
        }
        saved.insert(saved.cend(), controller);
    }

    conf.insert_or_assign("saved", saved);
}

void Config::writeToFile(std::string_view path) {
    std::fstream out;
    out.open(path.data(), std::ios::out | std::ios::trunc);
    out << toml::toml_formatter(conf);
    out.close();
}

};  // namespace sys
