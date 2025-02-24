#ifndef __UI_HPP__
#define __UI_HPP__

#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "GLFW/glfw3.h"
#include "core/mediators/fanMediator.hpp"
#include "core/mediator.hpp"
#include "core/plotStrategy.hpp"
#include "imgui.h"

namespace gui {
class GuiManager {
   public:
    using FileDialogCallback = std::function<void(std::string const&)>;
    using GeneralCallback = std::function<void()>;

    static std::shared_ptr<ImVector<char const*>> extensions();

    void setMediator(std::shared_ptr<core::Mediator> mediator);
    void setStrategy(std::unique_ptr<core::PlotStrategy> strategy) {
        plot_stategy = std::move(strategy);
    }

    void setGPUName(std::string const& name) { gpu_name = std::move(name); }
    void setCPUName(std::string const& name) { cpu_name = std::move(name); }
    void setRenderSize(std::pair<int, int> const& w_size) {
        size = std::move(w_size);
    }

    void updateCPUCurrentTemp(float temp) { current_cpu_temp = temp; }
    void updateGPUCurrentTemp(float temp) { current_gpu_temp = temp; }

    void updateCurrentFanStats(std::size_t controller_idx, std::size_t fan_idx,
                               std::size_t speed, std::size_t rpm);

    template <typename... Args>
    void setCallbacks(Args&&... args) {
        static_assert(
            sizeof...(args) % 2 == 0,
            "Callbacks must be provided as pairs of (name, callback)");

        setCallbacksImpl(std::forward<Args>(args)...);
    }

    GuiManager(GuiManager const&) = delete;
    GuiManager(GuiManager&&) = delete;
    GuiManager& operator=(GuiManager const&) = delete;
    GuiManager& operator=(GuiManager&&) = delete;
    explicit GuiManager(std::shared_ptr<GLFWwindow> const& window,
                        std::shared_ptr<sys::System> system);

    void render();

    ~GuiManager();

   private:
    void renderMenuBar();
    void renderPlotButtons();
    void renderTable();
    void renderApplyButton();
    void renderColorForAll();
    void renderMonitoring();

    static void cleanup();
    void printPlot(std::size_t i, std::size_t j);

    template <typename Name, typename Callback, typename... Rest>
    void setCallbacksImpl(Name&& name, Callback&& callback, Rest&&... rest) {
        setSingleCallback(std::forward<Name>(name),
                          std::forward<Callback>(callback));
        if constexpr (sizeof...(rest) > 0) {
            setCallbacksImpl(
                std::forward<Rest>(rest)...);
        }
    }

    template <typename Callback>
    void setSingleCallback(std::string const&& name, Callback&& callback) {
        if constexpr (std::is_invocable_r_v<void, Callback,
                                            std::string const&>) {
            fileDialogCallbacks[name].emplace_back(
                FileDialogCallback(std::forward<Callback>(callback)));
        } else if constexpr (std::is_invocable_r_v<void, Callback>) {
            generalCallbacks[name].emplace_back(
                GeneralCallback(std::forward<Callback>(callback)));
        } else {
            static_assert(AlwaysFalse<Callback>::value,
                          "Unsupported callback type");
        }
    }
    template <typename>
    struct AlwaysFalse : std::false_type {};

    float current_cpu_temp = 0;
    float current_gpu_temp = 0;
    std::string gpu_name;
    std::string cpu_name;
    std::pair<int, int> size;
    std::unordered_map<std::string, std::vector<FileDialogCallback>>
        fileDialogCallbacks;
    std::unordered_map<std::string, std::vector<GeneralCallback>>
        generalCallbacks;
    std::unordered_map<std::size_t, std::pair<std::size_t, std::size_t>> stats;
    std::shared_ptr<core::Mediator> mediator;
    std::shared_ptr<sys::System> system;
    std::unordered_map<std::size_t, int> fanMods;
    std::unique_ptr<core::PlotStrategy> plot_stategy;
    std::unordered_map<std::size_t, std::array<float, 3>> colors;
    std::array<float, 3> color{0.5f, 0.5f, 0.5f};
};
}  // namespace gui
#endif  // !__UI_HPP__
