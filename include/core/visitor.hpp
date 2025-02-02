#ifndef __VISITOR_HPP__
#define __VISITOR_HPP__

#include <memory>

namespace core {

class PointPlotStrategy;
class BezierCurvePlotStrategy;

struct PlotVisitor {
    virtual ~PlotVisitor() = default;
    virtual void visit(PointPlotStrategy& strategy) = 0;
    virtual void visit(BezierCurvePlotStrategy& strategy) = 0;
};

}
#endif // !__VISITOR_HPP__
