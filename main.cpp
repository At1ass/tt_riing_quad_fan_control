#include "core/fan_controller.hpp"
#include "core/fan_mediator.hpp"
#include "core/logger.hpp"
#include "core/mediator.hpp"
#include "core/observer.hpp"
#include "core/pointPlotStrategy.hpp"
#include "gui/gtk_tray_manager.hpp"
#include "gui/ui.hpp"
#include "gui/window_manager.hpp"
#include "imgui.h"
#include "system/config.hpp"
#include "system/hidapi_wrapper.hpp"
#include "system/monitoring.hpp"
#include "system/vulkan.hpp"
#include <filesystem>
#include <functional>
#include <pthread.h>
#include <unistd.h>


auto main(int  /*argc*/, char**  /*argv*/) -> int
{
    core::Logger::log_.enableColorLogging(true);

    try {
        std::shared_ptr<sys::HidWrapper> wrapper;
        std::string path;

        auto win_manager = std::make_shared<gui::WindowManager>("Fan Control", 1280, 720);

        auto tray_manager = std::make_shared<gui::GTKTrayManager>();

        tray_manager->setOnToggleCallback([&](){
            static bool windowHidden = true;

            if (windowHidden) {
                core::Logger::log_(core::LogLevel::INFO) << "Restoring window from tray" << std::endl;
                win_manager->showWindow();
            } else {
                core::Logger::log_(core::LogLevel::INFO) << "Hiding window to tray" << std::endl;
                win_manager->hideWindow();
            }

            windowHidden = !windowHidden;
        });

        tray_manager->setOnQuitCallback([&](){
            core::Logger::log_(core::LogLevel::INFO) << "Quit requested from tray." << std::endl;
            win_manager->closeWindow(); // Закрываем GLFW
            tray_manager->cleanup();
        });

        win_manager->setOnCloseCallback([&](){
            core::Logger::log_(core::LogLevel::INFO) << "Window closed via close button." << std::endl;
            tray_manager->stop();
        });

        std::shared_ptr<sys::System> system;
        sys::Monitoring mon;
        wrapper = std::make_shared<sys::HidWrapper>();
        sys::Config::getInstance().setControllerNum(wrapper->controllersNum());

        std::string const home_dir(getenv("HOME"));
        path = home_dir + "/.config/config2.toml";

        if (std::filesystem::exists(path)) {
            try {
                system = sys::Config::getInstance().parseConfig(path);
            }
            catch (std::exception e) {
                core::Logger::log_(core::LogLevel::ERROR) << "Failed read config: " << e.what() << std::endl;
                std::terminate();
            }
            sys::Config::getInstance().printConfig(system);
        }
        else {
            system = std::make_shared<sys::System>();
            sys::Config::getInstance().initDummyFans(system);
            sys::Config::getInstance().printConfig(system);
        }


        std::shared_ptr<core::FanController> const fc = std::make_shared<core::FanController>(system, wrapper);

        std::shared_ptr<core::ObserverCPU> const cpu_o = std::make_shared<core::ObserverCPU>(fc);
        std::shared_ptr<core::ObserverGPU> const gpu_o = std::make_shared<core::ObserverGPU>(fc);

        mon.addObserver(cpu_o);
        mon.addObserver(gpu_o);

        sys::Vulkan::setupVulkan(*gui::GuiManager::extensions());
        sys::Vulkan::createVulkanSurface(win_manager->getWindow().get());

        win_manager->createFramebuffers();
        win_manager->hideWindow();

        std::shared_ptr<gui::GuiManager> const gui = std::make_shared<gui::GuiManager>(win_manager->getWindow());

        gui->setStrategy(std::make_unique<core::PointPlotStrategy>());
        auto mediator = std::make_shared<core::FanMediator>(gui, fc);

        gui->setMediator(mediator);
        fc->setMediator(mediator);

        mediator->notify(EventMessageType::Initialize, Message{});

        gui->setCallbacks(
                "onOpenFile", std::function<void(const std::string&)>([&](const std::string& filePath) {
                    tray_manager->openFileDialog([&](const std::string& selectedFile) {
                        core::Logger::log_(core::LogLevel::INFO) << "File selected: " << selectedFile << std::endl;
                        try {
                            auto new_system = sys::Config::getInstance().parseConfig(selectedFile);
                            sys::Config::getInstance().printConfig(new_system);
                            *system = *new_system;
                            path = std::move(selectedFile);
                            mon.fullUpdate();
                            fc->reloadAllFans();
                        }
                        catch (std::exception e) {
                            core::Logger::log_(core::LogLevel::ERROR) << "Failed reading config" << e.what() << std::endl;
                        }
                    });
                    core::Logger::log_(core::LogLevel::INFO) << "Opening file: " << filePath << std::endl;
                }),
                "onSaveFile", std::function<void(const std::string&)>([&](const std::string& filePath) {
                    tray_manager->saveFileDialog([&](const std::string& savedFile){
                        core::Logger::log_(core::LogLevel::INFO) << "File selected: " << savedFile << std::endl;
                        sys::Config::getInstance().updateConf(system);
                        sys::Config::getInstance().writeToFile(savedFile);
                    });
                    core::Logger::log_(core::LogLevel::INFO) << "Saving file: " << filePath << std::endl;
                }),
                "onApply", std::function<void()>([&](){
                    core::Logger::log_(core::LogLevel::INFO) << "Apply callback" << std::endl;
                    core::Logger::log_(core::LogLevel::INFO) << "Write to " << path << std::endl;
                    sys::Config::getInstance().updateConf(system);
                    sys::Config::getInstance().writeToFile(path);
                }),
                "onQuit", std::function<void()>([&]() {
                    core::Logger::log_(core::LogLevel::INFO) << "Quit requested." << std::endl;
                    win_manager->closeWindow();
                })
        );

        while (!win_manager->shouldClose())
        {
            win_manager->pollEvents();
            win_manager->createOrResize();
            gui->render();
        }
    }
    catch (const std::exception e) {
        core::Logger::log_(core::LogLevel::ERROR) << "Error" << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
