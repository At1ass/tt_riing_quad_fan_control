#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__
#include "toml.hpp"
#include <memory>
#include <string_view>
#include <vector>
#include <generator>

namespace sys {
    enum class MONITORING_MODE {
        MONITORING_CPU = 0,
        MONITORING_GPU
    };
    class FanSpeedData {
        public:
            FanSpeedData();
            void addSpeed(float s);
            void addTemp(float t);
            void updateData(std::vector<double> t, std::vector<double> s);
            std::vector<double>* getTData();
            std::vector<double>* getSData();
            std::pair<std::vector<double>, std::vector<double>> getData();
            double getSpeedForTemp(const float& temp);
            void resetData() { temps.clear(); speeds.clear();}
        private:
            std::vector<double> temps;
            std::vector<double> speeds;
    };

    class FanBezierData {
        public:
            FanBezierData();
            void addControlPoint(const std::pair<double, double>& cp);
            auto getData() -> std::array<std::pair<double, double>, 4>& ;
            void setData(const std::array<std::pair<double, double>, 4>& data) ;
            double getSpeedForTemp(const float& temp);
            int getIdx() {return idx;}
        private:
            std::pair<double, double> ComputeBezierAtT(double t) ;

            int idx = 0;
            std::array<std::pair<double, double>, 4> controlPoints;
    };

    class Fan {
        public:
            void addData(const FanSpeedData &data);
            void addBData(const FanBezierData &bdata);
            FanSpeedData& getData() { return data; }
            FanBezierData& getBData() { return bdata; }
            void setMonitoringMode(const MONITORING_MODE mode);
            MONITORING_MODE getMonitoringMode() { return monitoring_mode; }
            void setIdx(size_t i) {idx = i;}
            size_t getIdx() {return idx;}
        private:
            MONITORING_MODE monitoring_mode = MONITORING_MODE::MONITORING_CPU;
            size_t idx = 0;
            FanSpeedData data;
            FanBezierData bdata;
    };

    class Controller {
        public:
            void addFan(const Fan &fan);
            void setIdx(size_t i) {idx = i;}
            size_t getIdx() {return idx;}
            std::vector<Fan>& getFans() { return fans; }
        private:
            size_t idx;
            std::vector<Fan> fans;

    };

    class System {
        public:
            void addController(const Controller &controllers);
            std::vector<Controller>& getControllers() { return controllers; }
        private:
            std::vector<Controller> controllers;
    };

    class Config {
        private:
            bool readed = false;
            toml::parse_result conf;
            int controllers_num = 0;
            Config() { }
            Config(const Config&) = delete;
            void operator=(const Config&) = delete;

        public:
            static Config& getInstance() {
                static Config instance;
                return instance;
            }
            int getControllersNum() {
                return controllers_num;
            }
            std::shared_ptr<sys::System> parseConfig(std::string_view path);
            void initDummyFans(const std::shared_ptr<sys::System>& system) const;
            static void printConfig(const std::shared_ptr<sys::System>& system);
            bool isReaded() { return readed; }
            void setControllerNum(int cnum) { controllers_num = cnum; }

            void updateConf(const std::shared_ptr<sys::System>& system);
            void writeToFile(std::string_view path);
    };
};
#endif // !__CONFIG_HPP__
