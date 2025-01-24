#include "system/vulkan.hpp"
#include "GLFW/glfw3.h"
#include "core/logger.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <cstdint>
#include <cstring>
#include <ostream>
#include <stdlib.h>

namespace sys {
    VkAllocationCallbacks*   Vulkan::g_Allocator_ = nullptr;
    VkInstance               Vulkan::g_Instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice         Vulkan::g_PhysicalDevice_ = VK_NULL_HANDLE;
    uint32_t                 Vulkan::g_QueueFamily_ = static_cast<uint32_t>(-1);
    VkDevice                 Vulkan::g_Device_ = VK_NULL_HANDLE;
    VkQueue                  Vulkan::g_Queue_ = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT Vulkan::g_DebugReport_ = VK_NULL_HANDLE;
    VkPipelineCache          Vulkan::g_PipelineCache_ = VK_NULL_HANDLE;
    VkDescriptorPool         Vulkan::g_DescriptorPool_ = VK_NULL_HANDLE;
    uint32_t                 Vulkan::g_MinImageCount_ = 2;
    bool                     Vulkan::g_SwapChainRebuild_ = false;
    VkSurfaceKHR             Vulkan::surface_ = nullptr;
    ImGui_ImplVulkanH_Window Vulkan::g_MainWindowData_;
    ImGui_ImplVulkanH_Window* Vulkan::wd_ = nullptr;



    static void checkVkResult(VkResult err)
    {
        if (err == 0) {
            return;
        }
        core::Logger::log_(core::LogLevel::ERROR) <<  "[vulkan] Error: VkResult = " << static_cast<size_t>(err) << std::endl;
        if (err < 0) {
            abort();
        }
    }

    static auto isExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension) -> bool
    {
        for (const VkExtensionProperties& p : properties) {
            if (strcmp(p.extensionName, extension) == 0) {
                return true;
            }
        }
        return false;
    }

    void Vulkan::setupVulkan(ImVector<const char*> instance_extensions) {
        VkResult err;
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
            vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
            properties.resize(properties_count);
            err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
            checkVkResult(err);

            // Enable required extensions
            if (isExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
                instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
            if (isExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
            {
                instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
                create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            }
#endif

            // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
            const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
            create_info.enabledLayerCount = 1;
            create_info.ppEnabledLayerNames = layers;
            instance_extensions.push_back("VK_EXT_debug_report");
#endif

            // Create Vulkan Instance
            create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.Size);
            create_info.ppEnabledExtensionNames = instance_extensions.Data;
            err = vkCreateInstance(&create_info, g_Allocator_, &g_Instance_);
            checkVkResult(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
            volkLoadInstance(g_Instance);
#endif

            // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
            auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
            IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
            VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
            debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debug_report_ci.pfnCallback = debug_report;
            debug_report_ci.pUserData = nullptr;
            err = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
            check_vk_result(err);
#endif
        }

        // Select Physical Device (GPU)
        g_PhysicalDevice_ = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance_);
        IM_ASSERT(g_PhysicalDevice_ != VK_NULL_HANDLE);

        // Select graphics queue family
        g_QueueFamily_ = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice_);
        IM_ASSERT(g_QueueFamily_ != (uint32_t)-1);

        // Create Logical Device (with 1 queue)
        {
            ImVector<const char*> device_extensions;
            device_extensions.push_back("VK_KHR_swapchain");

            // Enumerate physical device extension
            uint32_t properties_count = 0;
            ImVector<VkExtensionProperties> properties;
            vkEnumerateDeviceExtensionProperties(g_PhysicalDevice_, nullptr, &properties_count, nullptr);
            properties.resize(properties_count);
            vkEnumerateDeviceExtensionProperties(g_PhysicalDevice_, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
            if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
                device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

            const float queue_priority[] = { 1.0F };
            VkDeviceQueueCreateInfo queue_info[1] = {};
            queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[0].queueFamilyIndex = g_QueueFamily_;
            queue_info[0].queueCount = 1;
            queue_info[0].pQueuePriorities = queue_priority;
            VkDeviceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
            create_info.pQueueCreateInfos = queue_info;
            create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.Size);
            create_info.ppEnabledExtensionNames = device_extensions.Data;
            err = vkCreateDevice(g_PhysicalDevice_, &create_info, g_Allocator_, &g_Device_);
            checkVkResult(err);
            vkGetDeviceQueue(g_Device_, g_QueueFamily_, 0, &g_Queue_);
        }

        // Create Descriptor Pool
        // The example only requires a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)
        // If you wish to load e.g. additional textures you may need to alter pools sizes.
        {
            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1;
            pool_info.poolSizeCount = static_cast<uint32_t>IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            err = vkCreateDescriptorPool(g_Device_, &pool_info, g_Allocator_, &g_DescriptorPool_);
            checkVkResult(err);
        }
    }

    void Vulkan::createVulkanSurface(GLFWwindow *window) {
        VkResult const err = glfwCreateWindowSurface(g_Instance_, window, g_Allocator_, &surface_);
        checkVkResult(err);
    }

    void Vulkan::setupVulkanWindow(int width, int height) {
        wd_ = &g_MainWindowData_;
        wd_->Surface = surface_;

        // Check for WSI support
        VkBool32 res = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice_, g_QueueFamily_, wd_->Surface, &res);
        if (res != VK_TRUE)
        {
            core::Logger::log_(core::LogLevel::ERROR) << "Error no WSI support on physical device 0" << std::endl;
            exit(-1);
        }

        // Select Surface Format
        const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
        const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        wd_->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice_, wd_->Surface, requestSurfaceImageFormat, static_cast<size_t>IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

        // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
        VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
        VkPresentModeKHR const present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
        wd_->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice_, wd_->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
        //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

        // Create SwapChain, RenderPass, Framebuffer, etc.
        IM_ASSERT(g_MinImageCount_ >= 2);
        ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance_, g_PhysicalDevice_, g_Device_, wd_, g_QueueFamily_, g_Allocator_, width, height, g_MinImageCount_);
    }

    void Vulkan::imGuiInitInfo(GLFWwindow *window) {
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = g_Instance_;
        init_info.PhysicalDevice = g_PhysicalDevice_;
        init_info.Device = g_Device_;
        init_info.QueueFamily = g_QueueFamily_;
        init_info.Queue = g_Queue_;
        init_info.PipelineCache = g_PipelineCache_;
        init_info.DescriptorPool = g_DescriptorPool_;
        init_info.RenderPass = wd_->RenderPass;
        init_info.Subpass = 0;
        init_info.MinImageCount = g_MinImageCount_;
        init_info.ImageCount = wd_->ImageCount;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = g_Allocator_;
        init_info.CheckVkResultFn = checkVkResult;
        ImGui_ImplVulkan_Init(&init_info);
    }

    void Vulkan::createOrResizeWindow(int fb_width, int fb_height) {
        if (fb_width > 0 && fb_height > 0 && (g_SwapChainRebuild_ || g_MainWindowData_.Width != fb_width || g_MainWindowData_.Height != fb_height))
        {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount_);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance_, g_PhysicalDevice_, g_Device_, &g_MainWindowData_, g_QueueFamily_, g_Allocator_, fb_width, fb_height, g_MinImageCount_);
            Vulkan::g_MainWindowData_.FrameIndex = 0;
            g_SwapChainRebuild_ = false;
        }
        /*ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount);*/
    }

    void Vulkan::newFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
    }

    void Vulkan::frameRender(ImDrawData *draw_data) {
        VkResult err;

        VkSemaphore image_acquired_semaphore  = wd_->FrameSemaphores[wd_->SemaphoreIndex].ImageAcquiredSemaphore;
        VkSemaphore render_complete_semaphore = wd_->FrameSemaphores[wd_->SemaphoreIndex].RenderCompleteSemaphore;
        err = vkAcquireNextImageKHR(g_Device_, wd_->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd_->FrameIndex);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        {
            g_SwapChainRebuild_ = true;
            return;
        }
        checkVkResult(err);

        ImGui_ImplVulkanH_Frame* fd = &wd_->Frames[wd_->FrameIndex];
        {
            err = vkWaitForFences(g_Device_, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
            checkVkResult(err);

            err = vkResetFences(g_Device_, 1, &fd->Fence);
            checkVkResult(err);
        }
        {
            err = vkResetCommandPool(g_Device_, fd->CommandPool, 0);
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
            info.renderPass = wd_->RenderPass;
            info.framebuffer = fd->Framebuffer;
            info.renderArea.extent.width = wd_->Width;
            info.renderArea.extent.height = wd_->Height;
            info.clearValueCount = 1;
            info.pClearValues = &wd_->ClearValue;
            vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

        // Submit command buffer
        vkCmdEndRenderPass(fd->CommandBuffer);
        {
            VkPipelineStageFlags const wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &image_acquired_semaphore;
            info.pWaitDstStageMask = &wait_stage;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &fd->CommandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &render_complete_semaphore;

            err = vkEndCommandBuffer(fd->CommandBuffer);
            checkVkResult(err);
            err = vkQueueSubmit(g_Queue_, 1, &info, fd->Fence);
            checkVkResult(err);
        }
    }

    void Vulkan::framePresent() {
        if (g_SwapChainRebuild_) {
            return;
        }
        VkSemaphore render_complete_semaphore = wd_->FrameSemaphores[wd_->SemaphoreIndex].RenderCompleteSemaphore;
        VkPresentInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &render_complete_semaphore;
        info.swapchainCount = 1;
        info.pSwapchains = &wd_->Swapchain;
        info.pImageIndices = &wd_->FrameIndex;
        VkResult const err = vkQueuePresentKHR(g_Queue_, &info);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        {
            g_SwapChainRebuild_ = true;
            return;
        }
        checkVkResult(err);
        wd_->SemaphoreIndex = (wd_->SemaphoreIndex + 1) % wd_->SemaphoreCount; // Now we can use the next set of semaphores
    }

    void Vulkan::render(ImDrawData *draw_data, ImVec4 *clear_color) {
        wd_->ClearValue.color.float32[0] = clear_color->x * clear_color->w;
        wd_->ClearValue.color.float32[1] = clear_color->y * clear_color->w;
        wd_->ClearValue.color.float32[2] = clear_color->z * clear_color->w;
        wd_->ClearValue.color.float32[3] = clear_color->w;
        frameRender(draw_data);
        framePresent();
    }

    void Vulkan::imGuiVulkanShutdown() {
        VkResult err;
        err = vkDeviceWaitIdle(g_Device_);
        checkVkResult(err);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    void Vulkan::cleanUpVulkanWindow() {
        ImGui_ImplVulkanH_DestroyWindow(g_Instance_, g_Device_, &g_MainWindowData_, g_Allocator_);
    }

    void Vulkan::cleanUpVulkan()
    {
        vkDestroyDescriptorPool(g_Device_, g_DescriptorPool_, g_Allocator_);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
        // Remove the debug report callback
        auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
        f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

        vkDestroyDevice(g_Device_, g_Allocator_);
        vkDestroyInstance(g_Instance_, g_Allocator_);
    }
}
