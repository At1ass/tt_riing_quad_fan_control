#ifndef __VULKAN_HPP__
#define  __VULKAN_HPP__
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

class Vulkan {
public:
    Vulkan() = delete;

    static void SetupVulkan(ImVector<const char*> instance_extensions);
    static void CreateVulkanSurface(GLFWwindow *window);
    static void SetupVulkanWindow(int width, int height);
    static void ImGuiInitInfo(GLFWwindow *window);
    static void CreateOrResizeWindow(int fb_width, int fb_height);
    static void NewFrame();
    static void Render(ImDrawData *draw_data, ImVec4 *clear_color);
    static void ImGuiVulkanShutdown();
    static void CleanUpVulkanWindow();
    static void CleanUpVulkan();

    static ImGui_ImplVulkanH_Window g_MainWindowData;
    static uint32_t                 g_MinImageCount;

private:
    static void FrameRender(ImDrawData* draw_data);
    static void FramePresent();
    static VkSurfaceKHR surface;
    static VkAllocationCallbacks*   g_Allocator;
    static VkInstance               g_Instance;
    static VkPhysicalDevice         g_PhysicalDevice;
    static uint32_t                 g_QueueFamily;
    static VkDevice                 g_Device;
    static VkQueue                  g_Queue;
    static VkDebugReportCallbackEXT g_DebugReport;
    static VkPipelineCache          g_PipelineCache;
    static VkDescriptorPool         g_DescriptorPool;
    static ImGui_ImplVulkanH_Window* wd;
    static bool                     g_SwapChainRebuild;
};

#endif // !__VULKAN_HPP__
