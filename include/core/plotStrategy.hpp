#ifndef __PLOT_STRATEGY__
#define __PLOT_STRATEGY__

#include <memory>

#include "core/mediators/fanMediator.hpp"
#include "core/visitor.hpp"
#include "system/controllerData.hpp"

namespace core {

class PlotStrategy {
   public:
    PlotStrategy(PlotStrategy const&) = default;
    PlotStrategy(PlotStrategy&&) = delete;
    PlotStrategy& operator=(PlotStrategy const&) = default;
    PlotStrategy& operator=(PlotStrategy&&) = delete;
    virtual ~PlotStrategy() = default;

    virtual void plot(
        std::size_t i, std::size_t j,
        std::variant<FanData, std::array<std::pair<double, double>, 4>> data,
        std::shared_ptr<sys::System> system) = 0;
    virtual void accept(PlotVisitor& visitor) = 0;

   protected:
    PlotStrategy() = default;
};

}  // namespace core
#endif  // !__PLOT_STRATEGY__
