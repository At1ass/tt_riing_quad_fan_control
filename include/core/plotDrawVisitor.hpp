#ifndef __PLOT_DRAW_VISITOR__
#define __PLOT_DRAW_VISITOR__

#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/pointPlotStrategy.hpp"
#include "core/bezierCurvePlotStrategy.hpp"
#include "core/visitor.hpp"
#include <memory>
#include <vector>

namespace core {
    struct PlotDrawVisitor : public PlotVisitor {
        PlotDrawVisitor(
            int i,
            int j,
            std::array<std::pair<double, double>, 4> bezierData,
            std::vector<double> temps,
            std::vector<double> speeds,
            std::shared_ptr<Mediator> mediator
        ) :
        i(i),
        j(j),
        bezierData(std::move(bezierData)),
        temps(std::move(temps)),
        speeds(std::move(speeds)),
        mediator(std::move(mediator))
        { }

        void visit(PointPlotStrategy& strategy) {
            core::Logger::log_(LogLevel::INFO) << "Visit PointPlotStrategy" << std::endl;
            strategy.plot(i, j, fanData{temps, speeds}, std::static_pointer_cast<FanMediator>(mediator));
        }
        void visit(BezierCurvePlotStrategy& strategy) {
            core::Logger::log_(LogLevel::INFO) << "Visit BezierCurvePlotStrategy" << std::endl;
            strategy.plot(i, j, bezierData, std::static_pointer_cast<FanMediator>(mediator));
        }

        private:
            int i, j;
            std::array<std::pair<double, double>, 4> bezierData;
            std::vector<double> temps;
            std::vector<double> speeds;
            std::shared_ptr<Mediator> mediator;
    };
}

#endif // !__PLOT_DRAW_VISITOR__
