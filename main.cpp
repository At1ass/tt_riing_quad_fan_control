#include <pthread.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>

#include "core/commands/compositeCommand.hpp"
#include "core/commands/rainbowColorCommand.hpp"
#include "core/commands/rainbowColorFadeCommand.hpp"
#include "core/commands/staticColorCommand.hpp"
#include "core/effectsEngine.hpp"
#include "core/fanController.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "core/mediators/fanMediator.hpp"
#include "core/observer.hpp"
#include "core/observers/ui/uiObserver.hpp"
#include "core/strategies/bezierCurvePlotStrategy.hpp"
#include "core/strategies/pointPlotStrategy.hpp"
#include "gui/gtkTrayManager.hpp"
#include "gui/ui.hpp"
#include "gui/windowManager.hpp"
#include "imgui.h"
#include "include/core/commands/staticColorCommand.hpp"
#include "system/CPUController.hpp"
#include "system/config.hpp"
#include "system/controllers/ttRiingQuadController.hpp"
#include "system/deviceController.hpp"
#include "system/monitoring.hpp"
#include "system/vulkan.hpp"

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;
constexpr int INTERVAL = 1100;

auto makeEngine() -> std::unique_ptr<core::EffectsEngine> {
    auto engine = std::make_unique<core::EffectsEngine>();

    engine->addEffect(
        std::make_unique<core::RainbowColorCommand>(std::chrono::seconds(2)));

    engine->addEffect(std::make_unique<core::RainbowColorFadeCommand>(
        std::chrono::seconds(2)));

    engine->addEffect(std::make_unique<core::StaticColorCommand>(
        0, 0, 0, std::chrono::steady_clock::duration::max()));

    auto composite_effect = std::make_unique<core::CompositeCommand>();

    composite_effect->addCommand(std::make_unique<core::StaticColorCommand>(
        255, 0, 0, std::chrono::seconds(10)));
    composite_effect->addCommand(std::make_unique<core::StaticColorCommand>(
        0, 255, 0, std::chrono::seconds(10)));

    engine->addEffect(std::move(composite_effect));
    engine->setActiveEffect(0);

    return engine;
}

auto main(int /*argc*/, char** /*argv*/) -> int {
    core::Logger::log.enableColorLogging(true);

    try {
        // For other controllers support, need create detect controllers class
        // and work with it
        std::shared_ptr<sys::TTRiingQuadController> wrapper;
        std::string path;

        auto win_manager =
            std::make_shared<gui::WindowManager>("Fan Control", WIDTH, HEIGHT);

        auto tray_manager = std::make_shared<gui::GTKTrayManager>();

        tray_manager->setCallbacks(
            "onToggle", std::function<void()>([&]() {
                bool window_hidden = win_manager->windowHided();

                if (window_hidden) {
                    core::Logger::log(core::LogLevel::INFO)
                        << "Restoring window from tray" << std::endl;
                    win_manager->showWindow();
                } else {
                    core::Logger::log(core::LogLevel::INFO)
                        << "Hiding window to tray" << std::endl;
                    win_manager->hideWindow();
                }

                window_hidden = !window_hidden;
            }),
            "onQuit", std::function<void()>([&]() {
                core::Logger::log(core::LogLevel::INFO)
                    << "Quit requested from tray." << std::endl;
                win_manager->closeWindow();
                tray_manager->cleanup();
            }));

        win_manager->setOnCloseCallback([&]() {
            core::Logger::log(core::LogLevel::INFO)
                << "Window closed via close button." << std::endl;
            win_manager->hideWindow();
        });

        std::shared_ptr<sys::System> system;
        sys::Monitoring mon(std::make_unique<sys::CPUController>(),
                            std::make_unique<sys::GPUController>(),
                            std::chrono::seconds(2));

        auto hidapi = std::make_unique<sys::HidApi>();
        wrapper =
            std::make_shared<sys::TTRiingQuadController>(std::move(hidapi));
        sys::Config::getInstance().setControllerNum(wrapper->controllersNum());

        system = sys::Config::getInstance().parseConfig(path);
        sys::Config::getInstance().printConfig(system);

        std::shared_ptr<core::FanController> const FC =
            std::make_shared<core::FanController>(system, wrapper,
                                                  std::move(makeEngine()));

        std::shared_ptr<core::ObserverCPU> const CPU_O =
            std::make_shared<core::ObserverCPU>(FC);
        std::shared_ptr<core::ObserverGPU> const GPU_O =
            std::make_shared<core::ObserverGPU>(FC);

        mon.addObserver(CPU_O);
        mon.addObserver(GPU_O);

        sys::Vulkan::setupVulkan(*gui::GuiManager::extensions());
        sys::Vulkan::createVulkanSurface(win_manager->getWindow().get());

        win_manager->createFramebuffers();
        win_manager->hideWindow();

        std::shared_ptr<gui::GuiManager> const GUI =
            std::make_shared<gui::GuiManager>(win_manager->getWindow(), system);

        std::shared_ptr<core::ObserverUiCPU> const UI_CPU_O =
            std::make_shared<core::ObserverUiCPU>(GUI);
        std::shared_ptr<core::ObserverUiGPU> const UI_GPU_O =
            std::make_shared<core::ObserverUiGPU>(GUI);

        mon.addObserver(UI_CPU_O);
        mon.addObserver(UI_GPU_O);

        GUI->setGPUName(mon.getGpuName());
        GUI->setCPUName(mon.getCpuName());

        GUI->setStrategy(std::make_unique<core::PointPlotStrategy>());
        auto mediator = std::make_shared<core::FanMediator>(GUI, FC);

        GUI->setMediator(mediator);
        FC->setMediator(mediator);

        tray_manager->appendMenuItemsWithCallback(
            "Point curve", "onPointCurve", std::function<void()>([&FC, &GUI]() {
                FC->pointInfo();
                GUI->setStrategy(std::make_unique<core::PointPlotStrategy>());
            }),
            "Bezier curve", "onBezierCurve",
            std::function<void()>([&FC, &GUI]() {
                FC->bezierInfo();
                GUI->setStrategy(
                    std::make_unique<core::BezierCurvePlotStrategy>());
            }));

        GUI->setCallbacks(
            "onOpenFile",
            std::function<void(std::string const&)>(
                [&](std::string const& file_path) {
                    tray_manager->openFileDialog(
                        [&](std::string const& selected_file) {
                            core::Logger::log(core::LogLevel::INFO)
                                << "File selected: " << selected_file
                                << std::endl;
                            try {
                                auto new_system =
                                    sys::Config::getInstance().parseConfig(
                                        selected_file);
                                sys::Config::getInstance().printConfig(
                                    new_system);
                                *system = *new_system;
                                path = std::move(selected_file);
                            } catch (std::exception const& e) {
                                core::Logger::log(core::LogLevel::ERROR)
                                    << "Failed reading config: " << e.what()
                                    << std::endl;
                            }
                        });
                    core::Logger::log(core::LogLevel::INFO)
                        << "Opening file: " << file_path << std::endl;
                }),
            "onSaveFile",
            std::function<void(std::string const&)>(
                [&](std::string const& file_path) {
                    tray_manager->saveFileDialog(
                        [&](std::string const& saved_file) {
                            core::Logger::log(core::LogLevel::INFO)
                                << "File selected: " << saved_file << std::endl;
                            sys::Config::getInstance().updateConf(system);
                            sys::Config::getInstance().writeToFile(saved_file);
                        });
                    core::Logger::log(core::LogLevel::INFO)
                        << "Saving file: " << file_path << std::endl;
                }),
            "onApply", std::function<void()>([&]() {
                core::Logger::log(core::LogLevel::INFO)
                    << "Apply callback" << std::endl;
                core::Logger::log(core::LogLevel::INFO)
                    << "Write to opened config" << std::endl;
                sys::Config::getInstance().updateConf(system);
                sys::Config::getInstance().writeToFile();
            }),
            "onPointPlot", std::function<void()>([&]() {
                core::Logger::log(core::LogLevel::INFO)
                    << "Point Plot requested." << std::endl;
                FC->pointInfo();
            }),
            "onBezierPlot", std::function<void()>([&]() {
                core::Logger::log(core::LogLevel::INFO)
                    << "Point Plot requested." << std::endl;
                FC->bezierInfo();
            }),
            "onQuit", std::function<void()>([&]() {
                core::Logger::log(core::LogLevel::INFO)
                    << "Quit requested." << std::endl;
                win_manager->closeWindow();
            }));

        while (!win_manager->shouldClose()) {
            win_manager->pollEvents();
            win_manager->createOrResize();
            GUI->setRenderSize(win_manager->getWindowSize());
            GUI->render();
        }
    } catch (std::exception const& e) {
        core::Logger::log(core::LogLevel::ERROR)
            << "Error" << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
