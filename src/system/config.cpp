#include "system/config.hpp"
#include "core/logger.hpp"
#include "toml.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
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

    double FanSpeedData::getSpeedForTemp(const float& temp) {
        using pair = std::pair<double, double>;

        pair P1, P2;

        auto it = std::lower_bound(temps.begin(), temps.end(), temp);
        auto n = std::distance(temps.begin(), it);

        P1.first = temps[n];
        P2.first = temps[n - 1];
        P1.second = speeds[n];
        P2.second = speeds[n - 1];

        core::Logger::log_(core::LogLevel::INFO) << "Points: " << "[" << P1.first << ", " << P1.second << "] [" << P2.first << ", " << P2.second << "]" << std::endl;

        return P1.second + ( (P2.second - P1.second) / (P2.first - P1.first) ) * (temp - P1.first);
    }

    void FanBezierData::addControlPoint(const std::pair<double, double>& cp) {
        controlPoints[idx++] = std::move(cp);
    }

    auto FanBezierData::getData() -> std::array<std::pair<double, double>, 4>& {
        return controlPoints;
    }

    void FanBezierData::setData(const std::array<std::pair<double, double>, 4>& data) {
        controlPoints = std::move(data);
    }

    double FanBezierData::getSpeedForTemp(const float& temp) {
        using pair = std::pair<double, double> ;
        double t_low = 0.0, t_high = 1.0;
        double t_mid;
        double epsilon = 0.01;
        int max_iterations = 100;

        for (int i = 0; i < max_iterations; ++i) {
            t_mid = (t_low + t_high) / 2.0;
            pair P = ComputeBezierAtT(t_mid);

            if (std::abs(P.first - temp) < epsilon) {
                return P.second;  // Нашли нужное значение y
            }

            if (P.first < temp) {
                t_low = t_mid;
            } else {
                t_high = t_mid;
            }
        }

        return ComputeBezierAtT(t_mid).second;  // Возвращаем ближайшее найденное значение
    }

    std::pair<double, double> FanBezierData::ComputeBezierAtT(double t) {
        double u = 1.0 - t;
        double tt = t * t;
        double uu = u * u;
        double uuu = uu * u;
        double ttt = tt * t;

        std::pair<double, double> P;
        P.first = uuu * controlPoints[0].first + 3 * uu * t * controlPoints[1].first + 3 * u * tt * controlPoints[2].first + ttt * controlPoints[3].first;
        P.second = uuu * controlPoints[0].second + 3 * uu * t * controlPoints[1].second + 3 * u * tt * controlPoints[2].second + ttt * controlPoints[3].second;

        return P;
    }

    void Fan::addData(const FanSpeedData &data) {
        this->data = data;
    }

    void Fan::addBData(const FanBezierData &bdata) {
        this->bdata = bdata;
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
                        sys::FanBezierData bdata;
                        f.as_table()->for_each([&fan, &data, &bdata](const toml::key& key, auto&& val){
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
                            if (toml::is_array<decltype(val)> && key == "Control points") {
                                val.as_array()->for_each([&](auto &cp) {
                                    if (toml::is_table<decltype(cp)>) {
                                        double x,y;
                                        std::pair<double, double> cpoint;
                                        cp.as_table()->for_each([&](const toml::key& key, auto &&value){
                                            if (toml::is_floating_point<decltype(value)> && key == "x") {
                                                cpoint.first = value.as_floating_point()->get();
                                            }
                                            if (toml::is_floating_point<decltype(value)> && key == "y") {
                                                cpoint.second = value.as_floating_point()->get();
                                            }
                                        });
                                        bdata.addControlPoint(cpoint);
                                    } else {
                                        throw std::runtime_error("Incorrect control points structure");
                                    }
                                });
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

    void Config::initDummyFans(const std::shared_ptr<sys::System>& system) const {
        for(size_t i = 0; i < controllers_num; i++) {
            sys::Controller controller;
            controller.setIdx(i);
            for(size_t j = 0; j < 5; j++) {
                sys::Fan fan;
                fan.setIdx(j);
                fan.setMonitoringMode(sys::MONITORING_MODE::MONITORING_CPU);
                sys::FanSpeedData data;
                sys::FanBezierData bd;
                for(size_t k = 0; k <= 100; k+=5) {
                    data.addTemp(static_cast<float>(k));
                    data.addSpeed(50.0);
                }
                bd.setData(
                        {
                            std::make_pair<double, double>(0.0, 0.0),
                                std::make_pair<double, double>(40.0, 60.0),
                                std::make_pair<double, double>(60.0, 40.0),
                                std::make_pair<double, double>(100.0, 100.0)
                        }
                        );
                fan.addData(data);
                fan.addBData(bd);
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
                auto bdata = f.getBData().getData();
                int mon_mode = f.getMonitoringMode() == sys::MONITORING_MODE::MONITORING_GPU ? 1 : 0;
                toml::array temps;
                toml::array speeds;
                toml::array controlPoints;
                for (auto &&[t, s] : std::views::zip(data.first, data.second)) {
                    temps.push_back(t);
                    speeds.push_back(s);
                }
                for (auto &&cp : bdata) {
                    toml::table pos{{"x", cp.first}, {"y", cp.second}};
                    controlPoints.push_back(pos);
                }
                controller.insert(controller.cend(), toml::table{
                    {"Monitoring", mon_mode},
                    {"Temps", temps},
                    {"Speeds", speeds},
                    {"Control points", controlPoints}
                });
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
