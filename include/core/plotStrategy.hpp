#ifndef __PLOT_STRATEGY__
#define __PLOT_STRATEGY__

#include "core/fan_mediator.hpp"
#include <memory>
#include <vector>

namespace core {
    class PlotStrategy {
        public:
            virtual ~PlotStrategy() = default;

            virtual void plot(
                    int i,
                    int j,
                    std::vector<double> &temperatures,
                    std::vector<double> &speeds,
                    std::shared_ptr<core::FanMediator> mediator
                    ) = 0;
    };
}
#endif // !__PLOT_STRATEGY__
