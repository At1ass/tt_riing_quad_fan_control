#include "core/bezierCurvePlotStrategy.hpp"

#include <algorithm>
#include <array>

#include "core/logger.hpp"
#include "implot.h"
#include "system/controllerData.hpp"

static ImPlotPoint bezierGenerator(int idx, void* user_data) {
    double t = static_cast<double>(idx) / 100.0;  // t ∈ [0, 1]
    double u = 1.0 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    auto points =
        *static_cast<std::array<std::pair<double, double>, 4>*>(user_data);

    ImPlotPoint p;
    p.x = uuu * points[0].first + 3 * uu * t * points[1].first +
          3 * u * tt * points[2].first + ttt * points[3].first;
    p.y = uuu * points[0].second + 3 * uu * t * points[1].second +
          3 * u * tt * points[2].second + ttt * points[3].second;

    return p;
}

constexpr int const START_GRAPH = 0;
constexpr int const END_GRAPH = 100;
constexpr int const SAMPLES_NUM = 101;

namespace core {

void BezierCurvePlotStrategy::plot(
    std::size_t i, std::size_t j,
    std::variant<FanData, std::array<std::pair<double, double>, 4>> data,
    std::shared_ptr<sys::System> system) {
    auto cp = std::get<std::array<std::pair<double, double>, 4>>(data);
    if (ImPlot::BeginPlot("Fan Control (Bezier Curve) ", ImVec2(-1, -1),
                          ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes("Temperatures", "Speed", ImPlotAxisFlags_AutoFit,
                          ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxesLimits(START_GRAPH, END_GRAPH, START_GRAPH, END_GRAPH);

        ImPlot::PlotLineG("Bezier Curve", bezierGenerator, &cp, SAMPLES_NUM);

        std::array<double, 2> segment1_x = {cp[0].first, cp[1].first};
        std::array<double, 2> segment1_y = {cp[0].second, cp[1].second};
        ImPlot::PlotLine("Segment 1", segment1_x.data(), segment1_y.data(), 2);

        std::array<double, 2> segment2_x = {cp[2].first, cp[3].first};
        std::array<double, 2> segment2_y = {cp[2].second, cp[3].second};

        ImPlot::PlotLine("Segment 2", segment2_x.data(), segment2_y.data(), 2);

        for (int idx = 1; idx < 3; idx++) {
            if (ImPlot::DragPoint(idx, &cp[idx].first,
                                  &cp[idx].second,  // NOLINT
                                  ImVec4(1, 0, 0, 1), 4)) {
                cp[idx].first =
                    std::clamp(cp[idx].first, 0.0, 100.0);  // NOLINT
                cp[idx].second =
                    std::clamp(cp[idx].second, 0.0, 100.0);  // NOLINT
                if (system) {
                    std::ostringstream log_str;

                    log_str << "Control points: [ ";
                    for (auto&& [x, y] : cp) {
                        log_str << "[ " << x << ", " << y << " ] ";
                    }
                    log_str << "]" << std::endl;

                    // Логируем
                    Logger::log(LogLevel::INFO) << log_str.str() << std::endl;

                    system->getControllers()[i].getFans()[j].getBData().setData(
                        cp);
                }
            }
        }

        ImPlot::EndPlot();
    }
}

}  // namespace core
