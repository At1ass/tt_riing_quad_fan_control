#ifndef __WINDOW_MANAGER_HPP__
#define __WINDOW_MANAGER_HPP__

#include "GLFW/glfw3.h"
#include <functional>
#include <memory>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN

namespace gui {
    class WindowManager {
        public:
            using Callback = std::function<void()>;

            WindowManager(const char* title, int width, int height);

            void closeWindow();
            void hideWindow();
            bool shouldClose();
            static void pollEvents();
            void createOrResize();
            void showWindow();
            void createFramebuffers();
            static void cleanup();
            void getWindowSize(int& w, int& h);
            void setOnCloseCallback(Callback cb);
            std::shared_ptr<GLFWwindow> getWindow ();
        private:

            static void glfwErrorCallback(int error, const char* description);
            std::shared_ptr<GLFWwindow> window;
            Callback onCloseCallback = nullptr;
    };
}
#endif // !__WINDOW_MANAGER_HPP__
