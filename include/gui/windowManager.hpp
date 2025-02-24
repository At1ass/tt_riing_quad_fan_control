#ifndef __WINDOW_MANAGER_HPP__
#define __WINDOW_MANAGER_HPP__

#include <functional>
#include <memory>

#include "GLFW/glfw3.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN

namespace gui {

class WindowManager {
   public:
    using Callback = std::function<void()>;

    WindowManager(char const* title, int width, int height);

    void closeWindow();
    void hideWindow();
    bool shouldClose();
    static void pollEvents();
    void createOrResize();
    void showWindow();
    bool windowHided();
    void createFramebuffers();
    static void cleanup();
    void setOnCloseCallback(Callback cb);
    std::pair<int, int> getWindowSize();
    std::shared_ptr<GLFWwindow> getWindow();

   private:
    static void glfwErrorCallback(int error, char const* description);
    std::shared_ptr<GLFWwindow> window;
    Callback onCloseCallback = nullptr;
};

}  // namespace gui
#endif  // !__WINDOW_MANAGER_HPP__
