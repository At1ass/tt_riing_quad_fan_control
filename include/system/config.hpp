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
            void addSpeed(float s);
            void addTemp(float t);
            void updateData(std::vector<double> t, std::vector<double> s);
            std::vector<double>* getTData();
            std::vector<double>* getSData();
            std::pair<std::vector<double>, std::vector<double>> getData();

        private:
            std::vector<double> temps;
            std::vector<double> speeds;
    };

    class Fan {
        public:
            void addData(const FanSpeedData &data);
            FanSpeedData& getData() { return data; }
            void setMonitoringMode(const MONITORING_MODE mode);
            MONITORING_MODE getMonitoringMode() { return monitoring_mode; }
            void setIdx(size_t i) {idx = i;}
            size_t getIdx() {return idx;}
        private:
            MONITORING_MODE monitoring_mode;
            size_t idx;
            FanSpeedData data;
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
            int controllers_num;
            Config() { }
            Config(const Config&) = delete;
            void operator=(const Config&) = delete;

        public:
            static Config& getInstance() {
                static Config instance;
                return instance;
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
