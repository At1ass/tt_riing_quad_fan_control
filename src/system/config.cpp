#include "system/config.hpp"

#include <math.h>

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "core/logger.hpp"
#include "toml.hpp"

constexpr int const MAX_FAN = 5;

namespace sys {

auto Config::constructDefaultPath() -> std::string {
    std::string const HOME_DIR(getenv("HOME"));
    return HOME_DIR + "/.config/config2.toml";
}

auto Config::parseConfig(std::string_view path)
    -> std::shared_ptr<sys::System> {
    auto system = std::make_shared<sys::System>();
    std::string default_path = constructDefaultPath();

    if (path.empty()) {
        path = default_path;
    }

    if (!std::filesystem::exists(path)) {
        core::Logger::log(core::LogLevel::WARNING)
            << "Config file on path " << path
            << " not found. Create default configuration" << std::endl;
        std::cout << path << std::endl;
        initDummyFans(system);
        return system;
    }

    conf = toml::parse_file(path);
    auto* saved = conf["saved"].as_array();
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
