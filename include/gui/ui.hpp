#ifndef __UI_HPP__
#define __UI_HPP__

#include "GLFW/glfw3.h"
#include "core/fan_mediator.hpp"
#include "imgui.h"
#include "implot.h"
#include "core/mediator.hpp"
#include "core/plotStrategy.hpp"
#include "system/vulkan.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <unordered_map>

namespace gui {
    class GuiManager {
        public:
            using FileDialogCallback = std::function<void(const std::string&)>;
            using GeneralCallback = std::function<void()>;

            //GLFW
            static std::shared_ptr<ImVector<const char*>> extensions();

            void setMediator(std::shared_ptr<core::FanMediator> mediator);
            void setStrategy(std::unique_ptr<core::PlotStrategy> strategy) {
                plot_stategy = std::move(strategy);
            }
            void updateGraphData(
                int controller_idx,
                int fan_idx,
                std::variant<fanData, std::array<std::pair<double, double>, 4>> data
            );

            void updateFanMonitoringMods(
                int controller_idx,
                int fan_idx,
                const int& mode
            );

            template <typename... Args>
                void setCallbacks(Args&&... args) {
                    static_assert(sizeof...(args) % 2 == 0, "Callbacks must be provided as pairs of (name, callback)");

                    setCallbacksImpl(std::forward<Args>(args)...);
                }

            GuiManager(const std::shared_ptr<GLFWwindow>& window);

            void render();

            ~GuiManager();

        private:

            static void cleanup();
            void printPlot(int i, int j);

            template <typename Name, typename Callback, typename... Rest>
                void setCallbacksImpl(Name&& name, Callback&& callback, Rest&&... rest) {
                    setSingleCallback(std::forward<Name>(name), std::forward<Callback>(callback));
                    if constexpr (sizeof...(rest) > 0) {
                        setCallbacksImpl(std::forward<Rest>(rest)...); // Рекурсивная обработка
                    }
                }

            template <typename Callback>
                void setSingleCallback(const std::string&& name, Callback&& callback) {
                    if constexpr (std::is_invocable_r_v<void, Callback, const std::string&>) {
                        fileDialogCallbacks[name].emplace_back(FileDialogCallback(std::forward<Callback>(callback)));
                    } else if constexpr (std::is_invocable_r_v<void, Callback>) {
                        generalCallbacks[name].emplace_back(GeneralCallback(std::forward<Callback>(callback)));
                    } else {
                        static_assert(AlwaysFalse<Callback>::value, "Unsupported callback type");
                    }
                }
            template <typename>
                struct AlwaysFalse : std::false_type {};

            std::unordered_map<std::string, std::vector<FileDialogCallback>> fileDialogCallbacks;
            std::unordered_map<std::string, std::vector<GeneralCallback>> generalCallbacks;
            std::shared_ptr<core::FanMediator> mediator;
            std::unordered_map<int, std::pair<std::vector<double>, std::vector<double>>> graphData;
            std::unordered_map<int, std::array<std::pair<double, double>, 4>> bezierData;
            std::unordered_map<int, int> fanMods;
            std::unique_ptr<core::PlotStrategy> plot_stategy;
    };
}
#endif // !__UI_HPP__
