#include "core/pointPlotStrategy.hpp"

#include <algorithm>
#include <sstream>

#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "implot.h"

constexpr int const START_GRAPH = 0;
constexpr int const END_GRAPH = 100;
constexpr int const SAMPLES_NUM = 101;

namespace core {

void PointPlotStrategy::plot(
    std::size_t i, std::size_t j,
    std::variant<FanData, std::array<std::pair<double, double>, 4>> data,
    std::shared_ptr<core::Mediator> mediator) {
    auto d = std::get<FanData>(data);
    auto temperatures = d.t;
    auto speeds = d.s;
    if (ImPlot::BeginPlot("Fan Control", ImVec2(-1, -1),
                          ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes("Temperature", "Speed", ImPlotAxisFlags_AutoFit,
                          ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxesLimits(START_GRAPH, END_GRAPH, START_GRAPH, END_GRAPH);

        ImPlot::PlotLine("Curve", temperatures.data(), speeds.data(),
                         static_cast<int>(temperatures.size()));

        for (std::int32_t idx = 0; idx < temperatures.size(); ++idx) {
            if (ImPlot::DragPoint(idx, &temperatures[idx], &speeds[idx],
                                  ImVec4(1, 0, 0, 1), 4,
                                  ImPlotDragToolFlags_DisableX)) {
                temperatures[idx] = std::clamp(temperatures[idx], 0.0, 100.0);
                speeds[idx] = std::clamp(speeds[idx], 0.0, 100.0);
                if (mediator) {
                    std::ostringstream log_str;
                    log_str << "Temperatures: [ ";
                    for (auto&& t : temperatures) {
                        log_str << t << ", ";
                    }
                    log_str << "]\n";

                    log_str << "Speeds: [ ";
                    for (auto&& s : speeds) {
                        log_str << s << ", ";
                    }
                    log_str << "]\n";

                    // Логируем
                    Logger::log(LogLevel::INFO) << log_str.str() << std::endl;

                    // Отправляем сообщение через медиатор
                    mediator->notify(EventMessageType::UPDATE_GRAPH,
                                     std::make_shared<DataMessage>(DataMessage{
                                         i, j, FanData{temperatures, speeds}}));
                }
            }
        }

        ImPlot::EndPlot();
    }
}

}  // namespace core
