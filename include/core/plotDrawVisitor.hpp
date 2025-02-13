#ifndef __PLOT_DRAW_VISITOR__
#define __PLOT_DRAW_VISITOR__

#include <memory>
#include <vector>

#include "core/bezierCurvePlotStrategy.hpp"
#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/pointPlotStrategy.hpp"
#include "core/visitor.hpp"

namespace core {

struct PlotDrawVisitor : public PlotVisitor {
    PlotDrawVisitor(std::size_t i, std::size_t j,
                    std::array<std::pair<double, double>, 4> bezier_data,
                    std::vector<double> temps, std::vector<double> speeds,
                    std::shared_ptr<Mediator> mediator)
        : i(i),
          j(j),
          bezierData(std::move(bezier_data)),
          temps(std::move(temps)),
          speeds(std::move(speeds)),
          mediator(std::move(mediator)) {}

    void visit(PointPlotStrategy& strategy) override {
        core::Logger::log(LogLevel::INFO)
            << "Visit PointPlotStrategy" << std::endl;
        strategy.plot(i, j, FanData{temps, speeds}, mediator);
    }
    void visit(BezierCurvePlotStrategy& strategy) override {
        core::Logger::log(LogLevel::INFO)
            << "Visit BezierCurvePlotStrategy" << std::endl;
        strategy.plot(i, j, bezierData, mediator);
    }

   private:
    std::size_t i, j;
    std::array<std::pair<double, double>, 4> bezierData;
    std::vector<double> temps;
    std::vector<double> speeds;
    std::shared_ptr<Mediator> mediator;
};

}  // namespace core
#endif  // !__PLOT_DRAW_VISITOR__
