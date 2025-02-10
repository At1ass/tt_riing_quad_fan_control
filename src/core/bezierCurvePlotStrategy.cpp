#include "core/bezierCurvePlotStrategy.hpp"
#include "core/logger.hpp"
#include "implot.h"

#include <algorithm>
#include <array>

static ImPlotPoint BezierGenerator(int idx, void* user_data) {
    double t = (double)idx / 100.0;  // t ∈ [0, 1]
    double u = 1.0 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    auto points = *static_cast<std::array<std::pair<double, double>, 4>*>(user_data);


    ImPlotPoint P;
    P.x = uuu * points[0].first + 3 * uu * t * points[1].first + 3 * u * tt * points[2].first + ttt * points[3].first;
    P.y = uuu * points[0].second + 3 * uu * t * points[1].second + 3 * u * tt * points[2].second + ttt * points[3].second;

    return P;
}

static double controlPoints[4][2] = {

    {10.0, 10.0},
    {40.0, 40.0},
    {70.0, 30.0},
    {90.0, 90.0}
};

namespace core {
            void BezierCurvePlotStrategy::plot(
                int i,
                int j,
                std::variant<fanData, std::array<std::pair<double, double>, 4>> data,
                std::shared_ptr<core::Mediator> mediator
            )
            {
                auto cp = std::get<std::array<std::pair<double, double>, 4>>(data);
                if (ImPlot::BeginPlot("Fan Control (Bezier Curve) ", ImVec2(-1, -1),
                            ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
                    ImPlot::SetupAxes("X", "Y", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxesLimits(0, 100, 0, 100);

                    ImPlot::PlotLineG("Bezier Curve", BezierGenerator, &cp, 101);

                    double segment1X[2] = { cp[0].first, cp[1].first };
                    double segment1Y[2] = { cp[0].second, cp[1].second };
                    ImPlot::PlotLine("Segment 1", segment1X, segment1Y, 2);

                    double segment2X[2] = { cp[2].first, cp[3].first };
                    double segment2Y[2] = { cp[2].second, cp[3].second };
                    ImPlot::PlotLine("Segment 2", segment2X, segment2Y, 2);

                    for (int idx = 1; idx < 3; idx++) {
                        if (ImPlot::DragPoint(idx, &cp[idx].first, &cp[idx].second, ImVec4(1, 0, 0, 1), 4)) {
                            cp[idx].first = std::clamp(cp[idx].first, 0.0, 100.0);
                            cp[idx].second = std::clamp(cp[idx].second, 0.0, 100.0);
                            if (mediator) {
                                std::ostringstream log_str;

                                log_str << "Control points: [ ";
                                for (auto &&[x, y] : cp) {
                                    log_str << "[ " << x << ", " << y << " ] ";
                                }
                                log_str << "]" << std::endl;

                                // Логируем
                                Logger::log_(LogLevel::INFO) << log_str.str() << std::endl;

                                // Отправляем сообщение через медиатор
                                mediator->notify(
                                        EventMessageType::UpdateGraph,
                                        std::make_shared<DataMessage>(DataMessage{i, j, cp})
                                        );
                            }
                        }
                    }

                    ImPlot::EndPlot();
                }

            }
}
