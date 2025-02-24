#ifndef __CONTROLLER_DATA_HPP__
#define __CONTROLLER_DATA_HPP__

#include <array>
#include <utility>
#include <vector>

namespace sys {
enum class MonitoringMode { MONITORING_CPU = 0, MONITORING_GPU };
class FanSpeedData {
   public:
    FanSpeedData();
    ~FanSpeedData() = default;
    FanSpeedData(FanSpeedData const&) = default;
    FanSpeedData& operator=(FanSpeedData const&) = default;
    FanSpeedData& operator=(FanSpeedData&& d) noexcept {
        speeds = std::move(d.speeds);
        temps = std::move(d.temps);
        return *this;
    }
    FanSpeedData(std::vector<double> temps, std::vector<double> speeds)
        : temps(std::move(temps)), speeds(std::move(speeds)) {}
    FanSpeedData(FanSpeedData && d) noexcept
        : speeds(std::move(d.speeds)), temps(std::move(d.temps)) {}
    void addSpeed(float s);
    void setSpeeds(std::vector<double>&& s) { speeds = std::move(s); }
    void addTemp(float t);
    void setTemps(std::vector<double>&& t) { temps = std::move(t); }
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
    ~FanBezierData() = default;
    FanBezierData(FanBezierData const&) = default;
    FanBezierData& operator=(FanBezierData const&) = default;
    FanBezierData& operator=(FanBezierData&& bd) noexcept {
        controlPoints = std::move(bd.controlPoints);
        return *this;
    }
    explicit FanBezierData(
        std::array<std::pair<double, double>, 4> control_points)
        : controlPoints(std::move(control_points)) {}
    FanBezierData(FanBezierData && bd) noexcept
        : controlPoints(std::move(bd.controlPoints)) {}
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
    void addData(FanSpeedData&& d) {
        // data.resetData();
        data = std::move(d);
    }
    void addBData(FanBezierData&& bd) { bdata = std::move(bd); }
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

   private:
    std::vector<Controller> controllers;
};

}  // namespace sys
#endif  // !__CONTROLLER_DATA_HPP__
