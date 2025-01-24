#ifndef __POINT_PLOT_STRATEGY__
#define __POINT_PLOT_STRATEGY__
#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "core/plotStrategy.hpp"
#include "implot.h"
#include <sstream>

namespace core {
    class PointPlotStrategy : public PlotStrategy {
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
#endif // !__POINT_PLOT_STRATEGY__
