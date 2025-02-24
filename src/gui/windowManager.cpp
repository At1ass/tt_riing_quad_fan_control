#include "gui/windowManager.hpp"

#include <cstdio>
#include <iostream>
#include <memory>
#include <print>
#include <stdexcept>
#include <utility>

#include "GLFW/glfw3.h"
#include "core/logger.hpp"
#include "system/vulkan.hpp"

namespace gui {

WindowManager::WindowManager(char const* title, int width, int height) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (glfwInit() == 0) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    auto deleter = [](GLFWwindow* ptr) {
        if (ptr != nullptr) {
            glfwDestroyWindow(ptr);
        }
        glfwTerminate();
    };
    window = std::shared_ptr<GLFWwindow>(
        glfwCreateWindow(width, height, title, nullptr, nullptr), deleter);

    if (!window) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowCloseCallback(
        window.get(), +[](GLFWwindow* window) {
            core::Logger::log(core::LogLevel::INFO)
                << "Close Callback" << std::endl;
            glfwSetWindowShouldClose(window, false);
            auto* manager =
                static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
            if ((manager != nullptr) && manager->onCloseCallback) {
                manager->onCloseCallback();
            }
        });

    glfwSetWindowUserPointer(window.get(), this);
    if (glfwVulkanSupported() == 0) {
        throw std::runtime_error("GLFW: Vulkan not supported");
    }
}

void WindowManager::closeWindow() { glfwSetWindowShouldClose(window.get(), 1); }

void WindowManager::hideWindow() {
    if (window) {
        glfwHideWindow(window.get());
        core::Logger::log(core::LogLevel::INFO)
            << "GLFW window hidden." << std::endl;
    }
}

auto WindowManager::shouldClose() -> bool {
    return glfwWindowShouldClose(window.get()) != 0;
}

auto WindowManager::windowHided() -> bool {
    return glfwGetWindowAttrib(window.get(), GLFW_VISIBLE) == 0;
}

void WindowManager::pollEvents() { glfwPollEvents(); }

void WindowManager::createOrResize() {
    int fb_width = 0;
    int fb_height = 0;
    glfwGetFramebufferSize(window.get(), &fb_width, &fb_height);
    sys::Vulkan::createOrResizeWindow(fb_width, fb_height);
}

void WindowManager::showWindow() {
    if (window) {
        glfwShowWindow(window.get());
        glfwFocusWindow(window.get());
        core::Logger::log(core::LogLevel::INFO)
            << "GLFW window shown." << std::endl;
    }
}

void WindowManager::createFramebuffers() {
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window.get(), &w, &h);
    sys::Vulkan::setupVulkanWindow(w, h);
}

void WindowManager::cleanup() { glfwTerminate(); }

void WindowManager::setOnCloseCallback(Callback cb) {
    onCloseCallback = std::move(cb);
}

std::pair<int, int> WindowManager::getWindowSize() {
    int w = 0;
    int h = 0;

    glfwGetWindowSize(window.get(), &w, &h);

    return {w, h};
}

auto WindowManager::getWindow() -> std::shared_ptr<GLFWwindow> {
    return window;
}

void WindowManager::glfwErrorCallback(int error, char const* description) {
    std::println(stderr, "GLFW Error {}: {}", error, description);
}

}  // namespace gui
