#include "system/vulkan.hpp"

#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>

#include "GLFW/glfw3.h"
#include "core/logger.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace sys {

VkAllocationCallbacks* Vulkan::g_allocator = nullptr;
VkInstance Vulkan::g_instance = VK_NULL_HANDLE;
VkPhysicalDevice Vulkan::g_physical_device = VK_NULL_HANDLE;
uint32_t Vulkan::g_queue_family = static_cast<uint32_t>(-1);
VkDevice Vulkan::g_device = VK_NULL_HANDLE;
VkQueue Vulkan::g_queue = VK_NULL_HANDLE;
VkDebugReportCallbackEXT Vulkan::g_debug_report = VK_NULL_HANDLE;
VkPipelineCache Vulkan::g_pipeline_cache = VK_NULL_HANDLE;
VkDescriptorPool Vulkan::g_descriptor_pool = VK_NULL_HANDLE;
uint32_t Vulkan::g_min_image_count = 2;
bool Vulkan::g_swap_chain_rebuild = false;
VkSurfaceKHR Vulkan::surface = nullptr;
ImGui_ImplVulkanH_Window Vulkan::g_main_window_data;
ImGui_ImplVulkanH_Window* Vulkan::wd = nullptr;

static void checkVkResult(VkResult err) {
    if (err == 0) {
        return;
    }
    core::Logger::log(core::LogLevel::ERROR)
        << "[vulkan] Error: VkResult = " << static_cast<size_t>(err)
        << std::endl;
    if (err < 0) {
        abort();
    }
}

static auto isExtensionAvailable(
    ImVector<VkExtensionProperties> const& properties, char const* extension)
    -> bool {
    for (VkExtensionProperties const& p : properties) {
        std::span<char const> ext_name(p.extensionName);
        if (strcmp(ext_name.data(), extension) == 0) {
            return true;
        }
    }
    return false;
}

void Vulkan::setupVulkan(ImVector<char const*> instance_extensions) {
    VkResult err = VK_SUCCESS;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
#endif

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate available extensions
        uint32_t properties_count = 0;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count,
                                               nullptr);
        properties.resize(static_cast<int>(properties_count));
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count,
                                                     properties.Data);
        checkVkResult(err);

        // Enable required extensions
        if (isExtensionAvailable(
                properties,
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            instance_extensions.push_back(
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (isExtensionAvailable(
                properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
            instance_extensions.push_back(
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |=
                VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

        // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto layers = std::array{"VK_LAYER_KHRONOS_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers.data();
        instance_extensions.push_back("VK_EXT_debug_report");
#endif

        // Create Vulkan Instance
        create_info.enabledExtensionCount =
            static_cast<uint32_t>(instance_extensions.Size);
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        err = vkCreateInstance(&create_info, g_allocator, &g_instance);
        checkVkResult(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
        volkLoadInstance(g_instance);
#endif

        // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vk_create_debug_report_callback_ext =
            reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
                vkGetInstanceProcAddr(  // NOLINT
                    g_instance, "vkCreateDebugReportCallbackEXT"));
        IM_ASSERT(f_vk_create_debug_report_callback_ext != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType =
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        err = f_vk_create_debug_report_callback_ext(
            g_instance, &debug_report_ci, g_allocator, &g_debug_report);
        checkVkResult(err);
#endif
    }

    // Select Physical Device (GPU)
    g_physical_device = ImGui_ImplVulkanH_SelectPhysicalDevice(g_instance);
    IM_ASSERT(g_physical_device != VK_NULL_HANDLE);

    // Select graphics queue family
    g_queue_family =
        ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_physical_device);
    IM_ASSERT(g_queue_family != (uint32_t)-1);

    // Create Logical Device (with 1 queue)
    {
        ImVector<char const*> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        // Enumerate physical device extension
        uint32_t properties_count = 0;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(g_physical_device, nullptr,
                                             &properties_count, nullptr);
        properties.resize(static_cast<int>(properties_count));
        vkEnumerateDeviceExtensionProperties(
            g_physical_device, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (isExtensionAvailable(properties,
                                 VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
            device_extensions.push_back(
                VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }
#endif

        std::array<const float, 1> queue_priority = {1.0F};
        std::array<VkDeviceQueueCreateInfo, 1> queue_info{};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_queue_family;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority.data();
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount =
            sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info.data();
        create_info.enabledExtensionCount =
            static_cast<uint32_t>(device_extensions.Size);
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        err = vkCreateDevice(g_physical_device, &create_info, g_allocator,
                             &g_device);
        checkVkResult(err);
        vkGetDeviceQueue(g_device, g_queue_family, 0, &g_queue);
    }

    // Create Descriptor Pool
    // The example only requires a single combined image sampler descriptor for
    // the font image and only uses one descriptor set (for that) If you wish to
    // load e.g. additional textures you may need to alter pools sizes.
    {
        std::array<VkDescriptorPoolSize, 1> pool_sizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = pool_sizes.size();
        pool_info.pPoolSizes = pool_sizes.data();
        err = vkCreateDescriptorPool(g_device, &pool_info, g_allocator,
                                     &g_descriptor_pool);
        checkVkResult(err);
    }
}

void Vulkan::createVulkanSurface(GLFWwindow* window) {
    VkResult const ERR =
        glfwCreateWindowSurface(g_instance, window, g_allocator, &surface);
    checkVkResult(ERR);
}

void Vulkan::setupVulkanWindow(int width, int height) {
    wd = &g_main_window_data;
    wd->Surface = surface;

    // Check for WSI support
    VkBool32 res = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_physical_device, g_queue_family,
                                         wd->Surface, &res);
    if (res != VK_TRUE) {
        core::Logger::log(core::LogLevel::ERROR)
            << "Error no WSI support on physical device 0" << std::endl;
        exit(-1);
    }

    // Select Surface Format
    std::array<VkFormat, 4> const REQUEST_SURFACE_IMAGE_FORMAT = {
        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
    std::array<VkColorSpaceKHR, 1> const REQUEST_SURFACE_COLOR_SPACE = {
        VK_COLORSPACE_SRGB_NONLINEAR_KHR};
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        g_physical_device, wd->Surface, REQUEST_SURFACE_IMAGE_FORMAT.data(),
        REQUEST_SURFACE_IMAGE_FORMAT.size(),
        *REQUEST_SURFACE_COLOR_SPACE.data());

    // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    std::array<VkPresentModeKHR, 3> const PRESENT_MODES = {
        VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR,
        VK_PRESENT_MODE_FIFO_KHR};
#else
    std::array<VkPresentModeKHR, 1> const PRESENT_MODES = {
        VK_PRESENT_MODE_FIFO_KHR};
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        g_physical_device, wd->Surface, &PRESENT_MODES[0],
        PRESENT_MODES.size());

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_min_image_count >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        g_instance, g_physical_device, g_device, wd, g_queue_family,
        g_allocator, width, height, g_min_image_count);
}

void Vulkan::imGuiInitInfo(GLFWwindow* window) {
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_instance;
    init_info.PhysicalDevice = g_physical_device;
    init_info.Device = g_device;
    init_info.QueueFamily = g_queue_family;
    init_info.Queue = g_queue;
    init_info.PipelineCache = g_pipeline_cache;
    init_info.DescriptorPool = g_descriptor_pool;
    init_info.RenderPass = wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_min_image_count;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = g_allocator;
    init_info.CheckVkResultFn = checkVkResult;
    ImGui_ImplVulkan_Init(&init_info);
}

void Vulkan::createOrResizeWindow(int fb_width, int fb_height) {
    if (fb_width > 0 && fb_height > 0 &&
        (g_swap_chain_rebuild || g_main_window_data.Width != fb_width ||
         g_main_window_data.Height != fb_height)) {
        ImGui_ImplVulkan_SetMinImageCount(g_min_image_count);
        ImGui_ImplVulkanH_CreateOrResizeWindow(
            g_instance, g_physical_device, g_device, &g_main_window_data,
            g_queue_family, g_allocator, fb_width, fb_height,
            g_min_image_count);
        Vulkan::g_main_window_data.FrameIndex = 0;
        g_swap_chain_rebuild = false;
    }
}

void Vulkan::newFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}

void Vulkan::frameRender(ImDrawData* draw_data) {
    VkResult err = VK_SUCCESS;

    VkSemaphore image_acquired_semaphore =
        wd->FrameSemaphores[wd->SemaphoreIndex]
            .ImageAcquiredSemaphore;  // NOLINT
    VkSemaphore render_complete_semaphore =
        wd->FrameSemaphores[wd->SemaphoreIndex]
            .RenderCompleteSemaphore;  // NOLINT
    err = vkAcquireNextImageKHR(g_device, wd->Swapchain, UINT64_MAX,
                                image_acquired_semaphore, VK_NULL_HANDLE,
                                &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        g_swap_chain_rebuild = true;
        return;
    }
    checkVkResult(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];  // NOLINT
    {
        err = vkWaitForFences(
            g_device, 1, &fd->Fence, VK_TRUE,
            UINT64_MAX);  // wait indefinitely instead of periodically checking
        checkVkResult(err);

        err = vkResetFences(g_device, 1, &fd->Fence);
        checkVkResult(err);
    }
    {
        err = vkResetCommandPool(g_device, fd->CommandPool, 0);
        checkVkResult(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        checkVkResult(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info,
                             VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags const WAIT_STAGE =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &WAIT_STAGE;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        checkVkResult(err);
        err = vkQueueSubmit(g_queue, 1, &info, fd->Fence);
        checkVkResult(err);
    }
}

void Vulkan::framePresent() {
    if (g_swap_chain_rebuild) {
        return;
    }
    VkSemaphore render_complete_semaphore =
        wd->FrameSemaphores[wd->SemaphoreIndex]
            .RenderCompleteSemaphore;  // NOLINT
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult const ERR = vkQueuePresentKHR(g_queue, &info);
    if (ERR == VK_ERROR_OUT_OF_DATE_KHR || ERR == VK_SUBOPTIMAL_KHR) {
        g_swap_chain_rebuild = true;
        return;
    }
    checkVkResult(ERR);
    wd->SemaphoreIndex =
        (wd->SemaphoreIndex + 1) %
        wd->SemaphoreCount;  // Now we can use the next set of semaphores
}

void Vulkan::render(ImDrawData* draw_data, ImVec4* clear_color) {
    wd->ClearValue.color.float32[0] = clear_color->x * clear_color->w;
    wd->ClearValue.color.float32[1] = clear_color->y * clear_color->w;
    wd->ClearValue.color.float32[2] = clear_color->z * clear_color->w;
    wd->ClearValue.color.float32[3] = clear_color->w;
    frameRender(draw_data);
    framePresent();
}

void Vulkan::imGuiVulkanShutdown() {
    VkResult err = VK_SUCCESS;
    err = vkDeviceWaitIdle(g_device);
    checkVkResult(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

void Vulkan::cleanUpVulkanWindow() {
    ImGui_ImplVulkanH_DestroyWindow(g_instance, g_device, &g_main_window_data,
                                    g_allocator);
}

void Vulkan::cleanUpVulkan() {
    vkDestroyDescriptorPool(g_device, g_descriptor_pool, g_allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vk_destroy_debug_report_callback_ext =
        reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(  // NOLINT
                g_instance, "vkDestroyDebugReportCallbackEXT"));
    f_vk_destroy_debug_report_callback_ext(g_instance, g_debug_report,
                                           g_allocator);
#endif  // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_device, g_allocator);
    vkDestroyInstance(g_instance, g_allocator);
}

}  // namespace sys
