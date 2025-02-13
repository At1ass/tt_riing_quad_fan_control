#ifndef __POINT_PLOT_STRATEGY__
#define __POINT_PLOT_STRATEGY__
#include "core/fan_mediator.hpp"
#include "core/mediator.hpp"
#include "core/plotStrategy.hpp"

namespace core {

class PointPlotStrategy : public PlotStrategy {
   public:
    void plot(
        std::size_t i, std::size_t j,
        std::variant<FanData, std::array<std::pair<double, double>, 4>> data,
        std::shared_ptr<core::Mediator> mediator) override;

    void accept(PlotVisitor& visitor) override { visitor.visit(*this); }
};

}  // namespace core
#endif  // !__POINT_PLOT_STRATEGY__
