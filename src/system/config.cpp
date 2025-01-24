#include "system/config.hpp"
#include "core/logger.hpp"
#include "toml.hpp"
#include <cstddef>
#include <sstream>
#include <stdexcept>

namespace sys {
    void FanSpeedData::addSpeed(float s) {
        speeds.push_back(static_cast<double>(s));
    }
    void FanSpeedData::addTemp(float t) {
        temps.push_back(static_cast<double>(t));
    }

    void FanSpeedData::updateData(std::vector<double> t, std::vector<double> s) {
        temps.clear();
        speeds.clear();
        temps = std::move(t);
        speeds = std::move(s);
    }

    auto FanSpeedData::getTData() -> std::vector<double>* {
        return &temps;
    }
    auto FanSpeedData::getSData() -> std::vector<double>* {
        return &speeds;
    }
    auto FanSpeedData::getData() -> std::pair<std::vector<double>, std::vector<double>> {
        return {temps, speeds};
    }


    void Fan::addData(const FanSpeedData &data) {
        this->data = data;
    }

    void Fan::setMonitoringMode(const MONITORING_MODE mode) {
        monitoring_mode = mode;
    }

    void Controller::addFan(const Fan &fan) {
        fans.push_back(fan);
    }

    void System::addController(const Controller &controller) {
        controllers.push_back(controller);
    }

    auto Config::parseConfig(std::string_view path) -> std::shared_ptr<sys::System> {
        conf = toml::parse_file(path);
        auto *saved = conf["saved"].as_array();
        auto system = std::make_shared<sys::System>();
        if (saved != nullptr) {
            readed = true;
            size_t i = 0;
            size_t j = 0;
            for (auto &&d : *saved) {
                sys::Controller controller;
                controller.setIdx(i++);
                j = 0;
                d.as_array()->for_each([&controller, &j](auto &f){
                    if (toml::is_table<decltype(f)>) {
                        sys::Fan fan;
                        sys::FanSpeedData data;
                        f.as_table()->for_each([&fan, &data](const toml::key& key, auto&& val){
                            if (toml::is_integer<decltype(val)> && key == "Monitoring") {
                                fan.setMonitoringMode(
                                    val.as_integer()->get() ? sys::MONITORING_MODE::MONITORING_GPU : sys::MONITORING_MODE::MONITORING_CPU
                                );
                            }
                            if (toml::is_array<decltype(val)> && key == "Temps") {
                                val.as_array()->for_each([&data](auto &&t) {
                                    if (toml::is_floating_point<decltype(t)>) {
                                        data.addTemp(t.as_floating_point()->get());
                                    }
                                    else {
                                        throw std::runtime_error("Incorrect temp");
                                    }
                                });
                            }
                            if (toml::is_array<decltype(val)> && key == "Speeds") {
                                val.as_array()->for_each([&data](auto &&s) {
                                    if (toml::is_floating_point<decltype(s)>) {
                                        data.addSpeed(s.as_floating_point()->get());
                                    }
                                    else {
                                        throw std::runtime_error("Incorrect speed");
                                    }
                                });
                            }
                        });
                        fan.setIdx(j++);
                        fan.addData(data);
                        controller.addFan(fan);
                    }
                });
                system->addController(controller);
            }
            return system;
        }
        initDummyFans(system);
        return system;

    }

    void Config::initDummyFans(const std::shared_ptr<sys::System>& system) const {
        for(size_t i = 0; i < controllers_num; i++) {
            sys::Controller controller;
            controller.setIdx(i);
            for(size_t j = 0; j < 5; j++) {
                sys::Fan fan;
                fan.setIdx(j);
                fan.setMonitoringMode(sys::MONITORING_MODE::MONITORING_CPU);
                sys::FanSpeedData data;
                for(size_t k = 0; k <= 100; k+=5) {
                    data.addTemp(static_cast<float>(k));
                    data.addSpeed(50.0);
                }
                fan.addData(data);
                controller.addFan(fan);
            }
            system->addController(controller);
        }
    }

    void Config::printConfig(const std::shared_ptr<sys::System>& system) {
        std::ostringstream log_str;
        log_str << "Parsed config\n";
        for(auto &&c : system->getControllers()) {
            log_str << "Controller\n";
            for(auto &&f : c.getFans()) {
                log_str << "Fan (Monitoring " << (f.getMonitoringMode() == sys::MONITORING_MODE::MONITORING_CPU ? "CPU" : "GPU") << "): [";
                auto data =  f.getData().getData();
                for(auto &&[t, s] : std::views::zip(data.first, data.second)) {
                    log_str << "[ " << t << ", " << s << "], ";
                }
                log_str << "]\n";
            }
        }

        core::Logger::log_(core::LogLevel::INFO) << log_str.str() << std::endl;
    }

    void Config::updateConf(const std::shared_ptr<sys::System>& system) {
        toml::array saved;

        for(auto &&c : system->getControllers()) {
            toml::array controller;
            for(auto &&f : c.getFans()) {
                auto data = f.getData().getData();
                int mon_mode = f.getMonitoringMode() == sys::MONITORING_MODE::MONITORING_GPU ? 1 : 0;
                toml::array temps;
                toml::array speeds;
                for (auto &&[t, s] : std::views::zip(data.first, data.second)) {
                    temps.push_back(t);
                    speeds.push_back(s);
                }
                controller.insert(controller.cend(), toml::table{{"Monitoring", mon_mode}, {"Temps", temps}, {"Speeds", speeds}});
            }
            saved.insert(saved.cend(), controller);
        }

        conf.insert_or_assign("saved", saved);
    }

    void Config::writeToFile(std::string_view path) {
        std::fstream out;
        out.open(path.data(), std::ios::out | std::ios::trunc);
        out  <<  toml::toml_formatter(conf);
        out.close();
    }
};
