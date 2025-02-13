#ifndef __VULKAN_HPP__
#define __VULKAN_HPP__
#include <vulkan/vulkan_core.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace sys {

class Vulkan {
   public:
    Vulkan() = delete;

    static void setupVulkan(ImVector<char const*> instance_extensions);
    static void createVulkanSurface(GLFWwindow* window);
    static void setupVulkanWindow(int width, int height);
    static void imGuiInitInfo(GLFWwindow* window);
    static void createOrResizeWindow(int fb_width, int fb_height);
    static void newFrame();
    static void render(ImDrawData* draw_data, ImVec4* clear_color);
    static void imGuiVulkanShutdown();
    static void cleanUpVulkanWindow();
    static void cleanUpVulkan();

    static ImGui_ImplVulkanH_Window g_main_window_data;
    static uint32_t g_min_image_count;

   private:
    static void frameRender(ImDrawData* draw_data);
    static void framePresent();
    static VkSurfaceKHR surface;
    static VkAllocationCallbacks* g_allocator;
    static VkInstance g_instance;
    static VkPhysicalDevice g_physical_device;
    static uint32_t g_queue_family;
    static VkDevice g_device;
    static VkQueue g_queue;
    static VkDebugReportCallbackEXT g_debug_report;
    static VkPipelineCache g_pipeline_cache;
    static VkDescriptorPool g_descriptor_pool;
    static ImGui_ImplVulkanH_Window* wd;
    static bool g_swap_chain_rebuild;
};

}  // namespace sys
#endif  // !__VULKAN_HPP__
