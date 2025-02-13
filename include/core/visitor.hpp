#ifndef __VISITOR_HPP__
#define __VISITOR_HPP__

#include <memory>

namespace core {

class PointPlotStrategy;
class BezierCurvePlotStrategy;

struct PlotVisitor {
    PlotVisitor(PlotVisitor const&) = default;
    PlotVisitor(PlotVisitor&&) = delete;
    PlotVisitor& operator=(PlotVisitor const&) = default;
    PlotVisitor& operator=(PlotVisitor&&) = delete;
    virtual ~PlotVisitor() = default;
    virtual void visit(PointPlotStrategy& strategy) = 0;
    virtual void visit(BezierCurvePlotStrategy& strategy) = 0;

   protected:
    PlotVisitor() = default;
};

}  // namespace core
#endif  // !__VISITOR_HPP__
