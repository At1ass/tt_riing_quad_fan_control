#include "gui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <sstream>
#include <utility>
#include <vector>

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

constexpr int const SHIFT = 10;
constexpr double const SCALE = 1.4F;
constexpr int const TABLE_COLUMNS = 5;
constexpr int const FAN_BUTTON_SIZE = 120;

namespace gui {
auto GuiManager::extensions() -> std::shared_ptr<ImVector<char const*>> {
    std::shared_ptr<ImVector<char const*>> extensions =
        std::make_shared<ImVector<char const*>>();
    uint32_t extensions_count = 0;
    char const** glfw_extensions =
        glfwGetRequiredInstanceExtensions(&extensions_count);
    std::span<char const*> glfw_ext_span(glfw_extensions, extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++) {
        extensions->push_back(glfw_ext_span[i]);
    }
    return extensions;
}

void GuiManager::setMediator(std::shared_ptr<core::Mediator> mediator) {
    this->mediator = std::move(mediator);
}

void GuiManager::updateGraphData(
    std::size_t controller_idx, std::size_t fan_idx,
    std::variant<FanData, std::array<std::pair<double, double>, 4>> data) {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, FanData>) {
                auto d = std::get<FanData>(data);
            } else if constexpr (std::is_same_v<
                                     T, std::array<std::pair<double, double>,
                                                   4>>) {
                auto d =
                    std::get<std::array<std::pair<double, double>, 4>>(data);
            }
        },
        data);
}

void GuiManager::updateCurrentFanStats(std::size_t controller_idx,
                                       std::size_t fan_idx, std::size_t speed,
                                       std::size_t rpm) {
    stats[controller_idx * SHIFT + fan_idx] = {speed, rpm};
}
void GuiManager::updateFanMonitoringMods(std::size_t controller_idx,
                                         std::size_t fan_idx, int const& mode) {
    fanMods[controller_idx * SHIFT + fan_idx] = mode;
}

GuiManager::GuiManager(std::shared_ptr<GLFWwindow> const& window,
                       std::shared_ptr<sys::System> system)
    : system(system) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;            // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiWindowFlags_NoBackground;  // Enable Multi-Viewport /
                                                      // Platform Windows
    io.ConfigFlags |= ImGuiWindowFlags_NoResize;
    io.FontGlobalScale = SCALE;

    ImGui::StyleColorsDark();

    sys::Vulkan::imGuiInitInfo(window.get());
}

void GuiManager::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) {
                core::Logger::log(core::LogLevel::INFO) << "Open" << std::endl;
                if (fileDialogCallbacks.contains("onOpenFile") != 0u) {
                    for (auto&& c : fileDialogCallbacks["onOpenFile"]) {
                        c("dummy_open_file");
                    }
                }
            }
            if (ImGui::MenuItem("Save to")) {
                core::Logger::log(core::LogLevel::INFO) << "Save" << std::endl;
                if (fileDialogCallbacks.contains("onSaveFile") != 0u) {
                    for (auto&& c : fileDialogCallbacks["onSaveFile"]) {
                        c("dummy_open_file");
                    }
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Quit")) {
            core::Logger::log(core::LogLevel::INFO) << "Quit" << std::endl;
            if (generalCallbacks.contains("onQuit") != 0u) {
                for (auto&& c : generalCallbacks["onQuit"]) {
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
            for (auto&& c : generalCallbacks["onPointPlot"]) {
                c();
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Bezier Plot")) {
        setStrategy(std::make_unique<core::BezierCurvePlotStrategy>());
        if (generalCallbacks.contains("onQuit") != 0u) {
            for (auto&& c : generalCallbacks["onBezierPlot"]) {
                c();
            }
        }
    }
}

void GuiManager::renderTable() {
    std::array<std::string, 2> items = {"CPU", "GPU"};
    std::span<std::string> items_span(items);

    if (ImGui::BeginTable("controllers", TABLE_COLUMNS)) {
        for (std::size_t i = 0;
             i < sys::Config::getInstance().getControllersNum(); i++) {
            ImGui::TableNextRow();
            for (std::size_t j = 0; j < 3; j++) {
                ImGui::TableSetColumnIndex(static_cast<int>(j));
                ImGui::PushID(static_cast<int>(i * 4 + j));
                if (ImGui::Button("Fan speed curve settings",
                                  ImVec2(FAN_BUTTON_SIZE, FAN_BUTTON_SIZE))) {
                    core::Logger::log(core::LogLevel::INFO)
                        << "Button" << std::endl;
                    ImGui::OpenPopup("fctl", ImGuiPopupFlags_AnyPopupLevel);
                }

                ImGui::Text("Speed: %d", stats[i * SHIFT + j].first);  // NOLINT
                ImGui::Text("Rpm: %d", stats[i * SHIFT + j].second);   // NOLINT

                ImGui::SetNextWindowSize(
                    ImVec2(static_cast<float>(size.first / 2),
                           static_cast<float>(size.second / 2)));
                if (ImGui::BeginPopupModal("fctl")) {
                    auto current_item = items_span[fanMods[i * SHIFT + j]];
                    if (ImGui::BeginCombo("custom combo", current_item.data(),
                                          ImGuiComboFlags_NoArrowButton)) {
                        for (int n = 0; n < items.size(); n++) {
                            bool is_selected = (current_item == items_span[n]);
                            if (ImGui::Selectable(items_span[n].data(),
                                                  is_selected)) {
                                current_item = items_span[n];
                                fanMods[i * SHIFT + j] = n;
                                mediator->notify(
                                    EventMessageType::UPDATE_MONITORING_MODE_UI,
                                    std::make_shared<ModeMessage>(
                                        ModeMessage{i, j, n}));
                            }
                            if (is_selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Close")) {
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::ColorEdit3("Fan color edit",
                                          colors[i * SHIFT + j].data())) {
                        if (mediator) {
                            auto c = colors[i * SHIFT + j];
                            mediator->notify(
                                EventMessageType::UPDATE_COLOR,
                                std::make_shared<ColorMessage>(ColorMessage{
                                    i, j, c[0], c[1], c[2], false}));
                        }
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
            for (auto&& c : generalCallbacks["onApply"]) {
                c();
            }
        }
    }
}

void GuiManager::renderMonitoring() {
    ImGui::Text("%s temp:", cpu_name.c_str());  // NOLINT

    ImGui::Text("%d °C", static_cast<int>(current_cpu_temp));  // NOLINT

    ImGui::Text("%s temp:", gpu_name.c_str());  // NOLINT

    ImGui::Text("%d °C", static_cast<int>(current_gpu_temp));  // NOLINT
}

void GuiManager::renderColorForAll() {
    if (ImGui::ColorEdit3("All Fan color edit", color.data())) {
        for (size_t i = 0; i < sys::Config::getInstance().getControllersNum();
             i++) {
            for (size_t j = 0; j < 5; j++) {
                colors[i * SHIFT + j] = color;
            }
        }

        if (mediator) {
            mediator->notify(EventMessageType::UPDATE_COLOR,
                             std::make_shared<ColorMessage>(ColorMessage{
                                 0, 0, color[0], color[1], color[2], true}));
        }
    }
}

void GuiManager::render() {
    sys::Vulkan::newFrame();
    ImGui::NewFrame();

    ImVec4 clear_color = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);
    {
        ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(size.first),
                                        static_cast<float>(size.second)));
        ImGui::Begin("Fan ctrl", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize);

        renderMenuBar();
        renderPlotButtons();
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 2));
        if (ImGui::BeginTable("FanControlTable", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableNextColumn();
            renderColorForAll();
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
    bool const IS_MINIMIZED =
        (draw_data->DisplaySize.x <= 0.0F || draw_data->DisplaySize.y <= 0.0F);
    if (!IS_MINIMIZED) {
        sys::Vulkan::render(draw_data, &clear_color);
    }
}

GuiManager::~GuiManager() { cleanup(); }

void GuiManager::cleanup() {
    sys::Vulkan::imGuiVulkanShutdown();
    ImGui::DestroyContext();

    sys::Vulkan::cleanUpVulkanWindow();
    sys::Vulkan::cleanUpVulkan();
}

void GuiManager::printPlot(std::size_t i, std::size_t j) {
    auto data = system->getControllers()[i].getFans()[j].getData().getData();
    auto bdata = system->getControllers()[i].getFans()[j].getBData().getData();
    auto temperatures = data.first;
    auto speeds = data.second;

    core::PlotDrawVisitor visitor(i, j, bdata, temperatures, speeds, mediator);

    plot_stategy->accept(visitor);
}

}  // namespace gui
