#ifndef __BEZIER_CURVE_PLOT_STRATEGY__
#define __BEZIER_CURVE_PLOT_STRATEGY__

#include "core/fan_mediator.hpp"
#include "core/plotStrategy.hpp"

namespace core {

class BezierCurvePlotStrategy : public PlotStrategy {
   public:
    void plot(
        std::size_t i, std::size_t j,
        std::variant<FanData, std::array<std::pair<double, double>, 4>> data,
        std::shared_ptr<core::Mediator> mediator) override;

    void accept(PlotVisitor& visitor) override { visitor.visit(*this); }
};

}  // namespace core

#endif  // __BEZIER_CURVE_PLOT_STRATEGY__
