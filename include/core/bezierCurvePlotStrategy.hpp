#ifndef __BEZIER_CURVE_PLOT_STRATEGY__
#define __BEZIER_CURVE_PLOT_STRATEGY__

#include "core/fan_mediator.hpp"
#include "core/plotStrategy.hpp"

namespace core {
    class BezierCurvePlotStrategy : public PlotStrategy {
        public:
            void plot(
                int i,
                int j,
                std::variant<fanData, std::array<std::pair<double, double>, 4>> data,
                std::shared_ptr<core::FanMediator> mediator
            ) override;

            void accept(
                    PlotVisitor& visitor
                    ) override {
                visitor.visit(*this);
            }
    };
}

#endif // __BEZIER_CURVE_PLOT_STRATEGY__
