#ifndef __VULKAN_HPP__
#define  __VULKAN_HPP__
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace sys {
    class Vulkan {
        public:
            Vulkan() = delete;

            static void setupVulkan(ImVector<const char*> instance_extensions);
            static void createVulkanSurface(GLFWwindow *window);
            static void setupVulkanWindow(int width, int height);
            static void imGuiInitInfo(GLFWwindow *window);
            static void createOrResizeWindow(int fb_width, int fb_height);
            static void newFrame();
            static void render(ImDrawData *draw_data, ImVec4 *clear_color);
            static void imGuiVulkanShutdown();
            static void cleanUpVulkanWindow();
            static void cleanUpVulkan();

            static ImGui_ImplVulkanH_Window g_MainWindowData_;
            static uint32_t                 g_MinImageCount_;

        private:
            static void frameRender(ImDrawData* draw_data);
            static void framePresent();
            static VkSurfaceKHR surface_;
            static VkAllocationCallbacks*   g_Allocator_;
            static VkInstance               g_Instance_;
            static VkPhysicalDevice         g_PhysicalDevice_;
            static uint32_t                 g_QueueFamily_;
            static VkDevice                 g_Device_;
            static VkQueue                  g_Queue_;
            static VkDebugReportCallbackEXT g_DebugReport_;
            static VkPipelineCache          g_PipelineCache_;
            static VkDescriptorPool         g_DescriptorPool_;
            static ImGui_ImplVulkanH_Window* wd_;
            static bool                     g_SwapChainRebuild_;
    };
}
#endif // !__VULKAN_HPP__
