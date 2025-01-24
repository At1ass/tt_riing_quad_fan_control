#include "gui/window_manager.hpp"
#include "GLFW/glfw3.h"
#include "core/logger.hpp"
#include "system/vulkan.hpp"
#include <cstdio>
#include <iostream>
#include <memory>
#include <print>
#include <stdexcept>
#include <utility>

namespace gui {
    WindowManager::WindowManager(const char* title, int width, int height) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (glfwInit() == 0) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Create window with Vulkan context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

        auto deleter = [](GLFWwindow* ptr) {
            if (ptr != nullptr) {
                glfwDestroyWindow(ptr);
            }
            glfwTerminate();
        };
        window = std::shared_ptr<GLFWwindow>(glfwCreateWindow(width, height, title, nullptr, nullptr), deleter);

        if (!window) {
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwSetWindowCloseCallback(window.get(), +[](GLFWwindow* window) {
            core::Logger::log_(core::LogLevel::INFO) << "Close Callback" << std::endl;
            auto* manager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
            if ((manager != nullptr) && manager->onCloseCallback) {
                manager->onCloseCallback();
            }
        });

        glfwSetWindowUserPointer(window.get(), this);
        if (glfwVulkanSupported() == 0)
        {
            throw std::runtime_error("GLFW: Vulkan not supported");
        }
    }

    void WindowManager::getWindowSize(int& w, int& h) {
        glfwGetWindowSize(window.get(), &w, &h);
    }
    void WindowManager::closeWindow() {
        glfwSetWindowShouldClose(window.get(), 1);
    }

    void WindowManager::hideWindow() {
        if (window) {
            glfwHideWindow(window.get());
            core::Logger::log_(core::LogLevel::INFO) << "GLFW window hidden." << std::endl;
        }
    }

    auto WindowManager::shouldClose() -> bool {
        return glfwWindowShouldClose(window.get()) != 0;
    }

    void WindowManager::pollEvents() {
        glfwPollEvents();
    }

    void WindowManager::createOrResize() {
        int fb_width;
        int fb_height;
        glfwGetFramebufferSize(window.get(), &fb_width, &fb_height);
        sys::Vulkan::createOrResizeWindow(fb_width, fb_height);
    }

    void WindowManager::showWindow() {
        if (window) {
            glfwShowWindow(window.get());
            glfwFocusWindow(window.get());
            core::Logger::log_(core::LogLevel::INFO) << "GLFW window shown." << std::endl;
        }
    }

    void WindowManager::createFramebuffers() {
        // Create Framebuffers
        int w;
        int h;
        glfwGetFramebufferSize(window.get(), &w, &h);
        sys::Vulkan::setupVulkanWindow(w, h);
    }

    void WindowManager::cleanup() {
        glfwTerminate();
    }

    void WindowManager::setOnCloseCallback(Callback cb) {
        onCloseCallback = std::move(cb);
    }

    auto WindowManager::getWindow () -> std::shared_ptr<GLFWwindow> {
        return window;
    }

    void WindowManager::glfwErrorCallback(int error, const char* description)
    {
        std::println(stderr, "GLFW Error {}: {}", error, description);
    }
}
