#include "gui/ui.hpp"
#include "GLFW/glfw3.h"
#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "imgui.h"
#include "implot.h"
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

    void GuiManager::setMediator(std::shared_ptr<core::FanMediator> mediator) {
        this->mediator = std::move(mediator);
    }

    void GuiManager::updateGraphData(int controller_idx, int fan_idx, const std::vector<double>& temperatures, const std::vector<double>& speeds) {
        graphData[controller_idx * 10 + fan_idx] = {temperatures, speeds};
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


    void GuiManager::render() {
        sys::Vulkan::newFrame();
        ImGui::NewFrame();

        ImVec4 clear_color = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);
        {
            static float const f = 0.0F;
            static int const counter = 0;
            const char* items[] = { "CPU", "GPU"};

            ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F));
            ImGui::SetNextWindowSize(ImVec2(1280, 720));
            ImGui::Begin(
                    "Fan ctrl",
                    nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize
                    );

            ImGui::Text("Application average");

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

            if (ImGui::BeginTable("controllers", 5)) {
                for (size_t i = 0; i < 4; i++) {
                    ImGui::TableNextRow();
                    for (size_t j = 0; j < 3; j++) {
                        ImGui::TableSetColumnIndex(j);
                        ImGui::PushID(i * 4 + j);
                        if (ImGui::Button("Button")) {
                            core::Logger::log_(core::LogLevel::INFO) << "Button" << std::endl;
                            ImGui::OpenPopup("fctl", ImGuiPopupFlags_AnyPopupLevel);
                        }
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

            if (ImGui::Button("Apply")) {
                if (generalCallbacks.contains("onApply") != 0u) {
                    for (auto &&c : generalCallbacks["onApply"]) {
                        c();
                    }
                }
            }

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
        plot_stategy->plot(i, j, temperatures, speeds, mediator);
    }
}
