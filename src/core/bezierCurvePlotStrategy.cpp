#include "core/bezierCurvePlotStrategy.hpp"
#include "core/logger.hpp"
#include "implot.h"

#include <algorithm>

static ImPlotPoint BezierGenerator(int idx, void* user_data) {
    double t = (double)idx / 100.0;  // t ∈ [0, 1]
    double u = 1.0 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    double (*points)[2] = static_cast<double (*)[2]>(user_data);

    ImPlotPoint P;
    P.x = uuu * points[0][0] + 3 * uu * t * points[1][0] + 3 * u * tt * points[2][0] + ttt * points[3][0];
    P.y = uuu * points[0][1] + 3 * uu * t * points[1][1] + 3 * u * tt * points[2][1] + ttt * points[3][1];

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
                std::vector<double>& temperatures,
                std::vector<double>& speeds,
                std::shared_ptr<core::FanMediator> mediator
            )
            {
                if (ImPlot::BeginPlot("Bezier Curve (PlotLineG)", ImVec2(-1, -1),
                            ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
                    ImPlot::SetupAxes("X", "Y", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxesLimits(0, 100, 0, 100);

                    ImPlot::PlotLineG("Bezier Curve", BezierGenerator, controlPoints, 101);

                    double segment1X[2] = { controlPoints[0][0], controlPoints[1][0] };
                    double segment1Y[2] = { controlPoints[0][1], controlPoints[1][1] };
                    ImPlot::PlotLine("Segment 1", segment1X, segment1Y, 2);

                    double segment2X[2] = { controlPoints[2][0], controlPoints[3][0] };
                    double segment2Y[2] = { controlPoints[2][1], controlPoints[3][1] };
                    ImPlot::PlotLine("Segment 2", segment2X, segment2Y, 2);

                    for (int idx = 1; idx < 3; idx++) {
                        if (ImPlot::DragPoint(idx, &controlPoints[idx][0], &controlPoints[idx][1], ImVec4(1, 0, 0, 1), 4)) {
                            controlPoints[idx][0] = std::clamp(controlPoints[idx][0], 10.0, 90.0);
                            controlPoints[idx][1] = std::clamp(controlPoints[idx][1], 10.0, 90.0);
                        }
                    }

                    ImPlot::EndPlot();
                }

            }
}
