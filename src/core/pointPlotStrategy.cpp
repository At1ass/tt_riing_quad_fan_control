#include "core/pointPlotStrategy.hpp"
#include "core/logger.hpp"
#include "implot.h"

#include <algorithm>
#include <sstream>

namespace core {
            void PointPlotStrategy::plot(
                int i,
                int j,
                std::vector<double>& temperatures,
                std::vector<double>& speeds,
                std::shared_ptr<core::FanMediator> mediator
            )
            {
                if (ImPlot::BeginPlot("Fan Control", ImVec2(-1, -1),
                            ImPlotFlags_NoLegend | ImPlotFlags_NoMenus))
                {
                    ImPlot::SetupAxes("X", "Y", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                    ImPlot::SetupAxesLimits(0, 100, 0, 100);

                    ImPlot::PlotLine("Curve",
                            temperatures.data(),
                            speeds.data(),
                            static_cast<int>(temperatures.size()));

                    for (size_t idx = 0; idx < temperatures.size(); ++idx) {
                        if (ImPlot::DragPoint(idx,
                                    &temperatures[idx],
                                    &speeds[idx],
                                    ImVec4(1, 0, 0, 1),
                                    4,
                                    ImPlotDragToolFlags_DisableX))
                        {
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
                                Logger::log_(LogLevel::INFO) << log_str.str() << std::endl;

                                // Отправляем сообщение через медиатор
                                mediator->notify(
                                        EventMessageType::UpdateGraph,
                                        std::make_shared<DataMessage>(DataMessage{i, j, temperatures, speeds})
                                        );
                            }
                        }
                    }

                    ImPlot::EndPlot();
                }
            }
}
