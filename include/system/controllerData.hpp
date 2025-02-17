#ifndef __CONTROLLER_DATA_HPP__
#define __CONTROLLER_DATA_HPP__

#include <array>
#include <vector>

namespace sys {
enum class MonitoringMode { MONITORING_CPU = 0, MONITORING_GPU };
class FanSpeedData {
   public:
    FanSpeedData();
    void addSpeed(float s);
    void addTemp(float t);
    void updateData(std::vector<double> t, std::vector<double> s);
    std::vector<double>* getTData();
    std::vector<double>* getSData();
    std::pair<std::vector<double>, std::vector<double>> getData();
    double getSpeedForTemp(float const& temp);
    void resetData() {
        temps.clear();
        speeds.clear();
    }

   private:
    std::vector<double> temps;
    std::vector<double> speeds;
};

class FanBezierData {
   public:
    FanBezierData();
    void addControlPoint(std::pair<double, double> const& cp);
    auto getData() -> std::array<std::pair<double, double>, 4>&;
    void setData(std::array<std::pair<double, double>, 4> const& data);
    double getSpeedForTemp(float const& temp);
    int getIdx() { return idx; }

   private:
    std::pair<double, double> computeBezierAtT(double t);

    int idx = 0;
    std::array<std::pair<double, double>, 4> controlPoints;
};

class Fan {
   public:
    void addData(FanSpeedData const& data);
    void addBData(FanBezierData const& bdata);
    FanSpeedData& getData() { return data; }
    FanBezierData& getBData() { return bdata; }
    void setMonitoringMode(MonitoringMode const MODE);
    MonitoringMode getMonitoringMode() { return monitoring_mode; }
    void setIdx(size_t i) { idx = i; }
    size_t getIdx() { return idx; }

   private:
    MonitoringMode monitoring_mode = MonitoringMode::MONITORING_CPU;
    size_t idx = 0;
    FanSpeedData data;
    FanBezierData bdata;
};

class Controller {
   public:
    void addFan(Fan const& fan);
    void setIdx(size_t i) { idx = i; }
    size_t getIdx() { return idx; }
    std::vector<Fan>& getFans() { return fans; }

   private:
    size_t idx;
    std::vector<Fan> fans;
};

class System {
   public:
    void addController(Controller const& controllers);
    std::vector<Controller>& getControllers() { return controllers; }
    // void updateConfiguration();

   private:
    std::vector<Controller> controllers;
};

}  // namespace sys
#endif  // !__CONTROLLER_DATA_HPP__
