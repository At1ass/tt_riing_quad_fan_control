#include "system/controllerData.hpp"

#include <cmath>

#include "core/logger.hpp"

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

}  // namespace sys
