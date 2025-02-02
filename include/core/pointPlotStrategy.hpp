#ifndef __POINT_PLOT_STRATEGY__
#define __POINT_PLOT_STRATEGY__
#include "core/fan_mediator.hpp"
#include "core/mediator.hpp"
#include "core/plotStrategy.hpp"

namespace core {
    class PointPlotStrategy : public PlotStrategy {
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
#endif // !__POINT_PLOT_STRATEGY__
