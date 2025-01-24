#include "core/pointPlotStrategy.hpp"

namespace core {
            void PointPlotStrategy::plot(
                int i,
                int j,
                std::vector<double>& temperatures,
                std::vector<double>& speeds,
                std::shared_ptr<core::FanMediator> mediator
            )
            {
                // Примерно повторяем ваш исходный код
                if (ImPlot::BeginPlot("Fan Control", ImVec2(-1, -1),
                            ImPlotFlags_NoLegend | ImPlotFlags_NoMenus))
                {
                    ImPlot::SetupAxesLimits(0, 100, 0, 100);

                    // Рисуем линию графика
                    ImPlot::PlotLine("Curve",
                            temperatures.data(),
                            speeds.data(),
                            static_cast<int>(temperatures.size()));

                    // Перебираем точки, чтобы DragPoint мог «тащить» их
                    for (size_t idx = 0; idx < temperatures.size(); ++idx) {
                        // Например, выключаем изменение по Y (ImPlotDragToolFlags_DisableY),
                        // если нужно тащить только по X, — но это на ваше усмотрение.
                        if (ImPlot::DragPoint(idx,
                                    &temperatures[idx],
                                    &speeds[idx],
                                    ImVec4(1, 0, 0, 1),
                                    4,
                                    ImPlotDragToolFlags_DisableY))
                        {
                            // Если действительно сдвинули точку, — логируем + шлём нотификацию
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
                                        Message{ i, j, temperatures, speeds }
                                        );
                            }
                        }
                    }

                    ImPlot::EndPlot();
                }
            }
}
