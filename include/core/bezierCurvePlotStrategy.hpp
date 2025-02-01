#ifndef __BEZIER_CURVE_PLOT_STRATEGY__
#define __BEZIER_CURVE_PLOT_STRATEGY__

#include "core/plotStrategy.hpp"

namespace core {
    class BezierCurvePlotStrategy : public PlotStrategy {
        public:
            void plot(
                int i,
                int j,
                std::vector<double>& temperatures,
                std::vector<double>& speeds,
                std::shared_ptr<core::FanMediator> mediator
            ) override;
    };
}

#endif // __BEZIER_CURVE_PLOT_STRATEGY__
