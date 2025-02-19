#include "system/config.hpp"

#include <math.h>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "core/logger.hpp"
#include "system/systemBuilder.hpp"
#include "toml.hpp"

namespace sys {
std::shared_ptr<System> Config::parseConfig(std::string_view path) {
    std::shared_ptr<System> system;
    try {
        system = builder.buildFromFile(path, controllers_num);
    } catch (std::exception const& e) {
        core::Logger::log(core::LogLevel::ERROR) << e.what() << std::endl;
        throw e;
    }
    return system;
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
