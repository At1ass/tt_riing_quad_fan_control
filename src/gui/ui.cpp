#include "gui/ui.hpp"
#include "GLFW/glfw3.h"
#include "core/bezierCurvePlotStrategy.hpp"
#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "core/plotDrawVisitor.hpp"
#include "core/pointPlotStrategy.hpp"
#include "imgui.h"
#include "implot.h"
#include "system/config.hpp"
#include "system/vulkan.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace gui {
    auto GuiManager::extensions() -> std::shared_ptr<ImVector<const char*>> {
        std::shared_ptr<ImVector<const char*>> extensions = std::make_shared<ImVector<const char *>>();
        uint32_t extensions_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
        for (uint32_t i = 0; i < extensions_count; i++) {
            extensions->push_back(glfw_extensions[i]);
        }
        return extensions;
    }

    void GuiManager::setMediator(std::shared_ptr<core::Mediator> mediator) {
        this->mediator = std::move(mediator);
    }

    void GuiManager::updateGraphData(int controller_idx, int fan_idx, std::variant<fanData, std::array<std::pair<double, double>, 4>> data) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, fanData>) {
                auto d = std::get<fanData>(data);
                graphData[controller_idx * 10 + fan_idx] = {d.t, d.s};
            } else if constexpr (std::is_same_v<T, std::array<std::pair<double, double>, 4>>) {
                auto d = std::get<std::array<std::pair<double, double>, 4>>(data);
                bezierData[controller_idx * 10 + fan_idx] = d;
            }

        }, data);
    }

    void GuiManager::updateFanMonitoringMods(int controller_idx, int fan_idx, const int& mode) {
        fanMods[controller_idx * 10 + fan_idx] = mode;
    }

    GuiManager::GuiManager(const std::shared_ptr<GLFWwindow>& window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiWindowFlags_NoBackground;       // Enable Multi-Viewport / Platform Windows
        io.ConfigFlags |= ImGuiWindowFlags_NoResize;
        io.FontGlobalScale = 1.4F;

        ImGui::StyleColorsDark();

        sys::Vulkan::imGuiInitInfo(window.get());
    }

    void GuiManager::renderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open")) {
                    core::Logger::log_(core::LogLevel::INFO) << "Open" << std::endl;
                    if (fileDialogCallbacks.contains("onOpenFile") != 0u) {
                        for (auto &&c : fileDialogCallbacks["onOpenFile"]) {
                            c("dummy_open_file");
                        }
                    }
                }
                if (ImGui::MenuItem("Save to")) {
                    core::Logger::log_(core::LogLevel::INFO) << "Save" << std::endl;
                    if (fileDialogCallbacks.contains("onSaveFile") != 0u) {
                        for (auto &&c : fileDialogCallbacks["onSaveFile"]) {
                            c("dummy_open_file");
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Quit")) {
                core::Logger::log_(core::LogLevel::INFO) << "Quit" << std::endl;
                if (generalCallbacks.contains("onQuit") != 0u) {
                    for (auto &&c : generalCallbacks["onQuit"]) {
                        c();
                    }
                }
            }
            ImGui::EndMenuBar();
        }
    }

    void GuiManager::renderPlotButtons() {
        if (ImGui::Button("Point Plot")) {
            setStrategy(std::make_unique<core::PointPlotStrategy>());
            if (generalCallbacks.contains("onQuit") != 0u) {
                for (auto &&c : generalCallbacks["onPointPlot"]) {
                    c();
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Bezier Plot")) {
            setStrategy(std::make_unique<core::BezierCurvePlotStrategy>());
            if (generalCallbacks.contains("onQuit") != 0u) {
                for (auto &&c : generalCallbacks["onBezierPlot"]) {
                    c();
                }
            }
        }
    }

    void GuiManager::renderTable() {
        const char* items[] = { "CPU", "GPU"};
        if (ImGui::BeginTable("controllers", 5)) {
            for (size_t i = 0; i < sys::Config::getInstance().getControllersNum(); i++) {
                ImGui::TableNextRow();
                for (size_t j = 0; j < 3; j++) {
                    ImGui::TableSetColumnIndex(j);
                    ImGui::PushID(i * 4 + j);
                    if (ImGui::Button("Fan speed curve settings", ImVec2(150, 150))) {
                        core::Logger::log_(core::LogLevel::INFO) << "Button" << std::endl;
                        ImGui::OpenPopup("fctl", ImGuiPopupFlags_AnyPopupLevel);
                    }
                    ImGui::SetNextWindowSize(ImVec2(size.first / 2, size.second / 2));
                    if (ImGui::BeginPopupModal("fctl")) {
                        const char *current_item = items[fanMods[i * 10 + j]];
                        if (ImGui::BeginCombo("custom combo", current_item, ImGuiComboFlags_NoArrowButton))
                        {
                            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                            {
                                bool is_selected = (current_item == items[n]);
                                if (ImGui::Selectable(items[n], is_selected)) {
                                    current_item = items[n];
                                    fanMods[i * 10 + j] = n;
                                    mediator->notify(EventMessageType::UpdateMonitoringModeUi, std::make_shared<ModeMessage>(ModeMessage{static_cast<int>(i), static_cast<int>(j), n}));

                                }
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Close")) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImPlot::CreateContext();
                        printPlot(i, j);
                        ImPlot::DestroyContext();
                        ImGui::EndPopup();

                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    }

    void GuiManager::renderApplyButton() {
        if (ImGui::Button("Apply")) {
            if (generalCallbacks.contains("onApply") != 0u) {
                for (auto &&c : generalCallbacks["onApply"]) {
                    c();
                }
            }
        }
    }

    void GuiManager::renderMonitoring() {
        ImGui::Text(
                "%s temp:",
                cpu_name.c_str()
                );

        ImGui::Text("%d °C",
                static_cast<int>(current_cpu_temp));

        ImGui::Text(
                "%s temp:",
                gpu_name.c_str()
                );

        ImGui::Text("%d °C",
                static_cast<int>(current_gpu_temp));
    }


    void GuiManager::render() {
        sys::Vulkan::newFrame();
        ImGui::NewFrame();

        ImVec4 clear_color = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);
        {
            static float const f = 0.0F;
            static int const counter = 0;

            ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F));
            ImGui::SetNextWindowSize(ImVec2(size.first, size.second));
            ImGui::Begin(
                    "Fan ctrl",
                    nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize
                    );

            renderMenuBar();
            renderPlotButtons();
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 2));
            if (ImGui::BeginTable("FanControlTable", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Fans", ImGuiTableColumnFlags_WidthFixed, 470);
                ImGui::TableNextColumn();
                renderTable();
                renderApplyButton();
                ImGui::TableNextColumn();
                renderMonitoring();
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
            ImGui::End();
        }


        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0F || draw_data->DisplaySize.y <= 0.0F);
        if (!is_minimized)
        {
            sys::Vulkan::render(draw_data, &clear_color);
        }
    }

    GuiManager::~GuiManager() {
        cleanup();
    }


    void GuiManager::cleanup() {
        sys::Vulkan::imGuiVulkanShutdown();
        ImGui::DestroyContext();

        sys::Vulkan::cleanUpVulkanWindow();
        sys::Vulkan::cleanUpVulkan();
    }

    void GuiManager::printPlot(int i, int j) {
        auto data = graphData[i * 10 + j];
        auto temperatures = data.first;
        auto speeds = data.second;
        auto bd = bezierData[i * 10 + j];

        core::PlotDrawVisitor visitor(i, j, bd, temperatures, speeds, mediator);

        plot_stategy->accept(visitor);
    }
}
