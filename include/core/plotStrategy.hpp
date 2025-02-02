#ifndef __PLOT_STRATEGY__
#define __PLOT_STRATEGY__

#include "core/visitor.hpp"
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
                    std::variant<fanData, std::array<std::pair<double, double>, 4>> data,
                    std::shared_ptr<core::FanMediator> mediator
                    ) = 0;
            virtual void accept(PlotVisitor& visitor) = 0;
    };
}
#endif // !__PLOT_STRATEGY__
