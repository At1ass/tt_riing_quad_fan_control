// Dear ImGui: standalone example application for Glfw + Vulkan

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering backend in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

#include "gdk/gdk.h"
#include "glib.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "implot.h"
#include "hidapi_wrapper.hpp"
#include "monitoring.hpp"
#include "toml.hpp"
#include "config.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <mutex>
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <array>
#include <string>
#include <unistd.h>
#include <vector>
#include <thread>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <libayatana-appindicator/app-indicator.h>
#include <gtk/gtk.h>
#include <pthread.h>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
#endif

// Data
static VkAllocationCallbacks*   g_Allocator = nullptr;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
{
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static void SetupVulkan(ImVector<const char*> instance_extensions)
{
    VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
#endif

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate available extensions
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
        check_vk_result(err);

        // Enable required extensions
        if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
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
        create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);
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
    g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
    IM_ASSERT(g_PhysicalDevice != VK_NULL_HANDLE);

    // Select graphics queue family
    g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
    IM_ASSERT(g_QueueFamily != (uint32_t)-1);

    // Create Logical Device (with 1 queue)
    {
        ImVector<const char*> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        // Enumerate physical device extension
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
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
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    wd->Surface = surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
}

static void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow()
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkResult err;

    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
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
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}

static GLFWwindow *window = NULL;
static hid_wrapper wrapper;
static config conf;
static std::atomic<bool> need_get_fan_data = false;
static bool need_open_file_dialog = false;
static bool is_open = false;
static std::atomic<bool> gtk_running = true;
static std::string path;// = "/home/at1ass/.config/config.toml";
std::shared_ptr<std::vector<std::vector<std::map<float, float>>>> fans;
std::shared_ptr<std::vector<int>> cpu_or_gpu;
std::mutex mutex;

// Функция для закрытия программы
static gboolean on_quit(GtkWidget *widget, gpointer data) {
    gtk_main_quit(); // Закрытие GTK приложения
    glfwSetWindowShouldClose(window, TRUE);
    return FALSE;
}

// Функция для сворачивания/разворачивания окна
static void on_toggle_window(GtkWidget *widget, gpointer data) {
    // Переключаем видимость окна
    if (glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
        glfwHideWindow(window);  // Скрыть окно
    } else {
        glfwShowWindow(window);  // Показать окно
    }
}

static void on_preset_clicked(GtkWidget *widget, gpointer data) {
    auto vals = (*conf.get_profiles())[std::string((char *)data)];
    size_t i = 0;

    for (auto &v : vals) {
        wrapper.send_to_controller(i++, v);
    }
    need_get_fan_data.store(true);
}

static void preset_section(GtkWidget **menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_label("Presets");
    gtk_widget_set_sensitive(item, FALSE);
    gtk_widget_set_margin_start(item, 0);
    gtk_widget_set_halign(item, GTK_ALIGN_START);

    gtk_menu_shell_append(GTK_MENU_SHELL(*menu), item);

    auto profiles = conf.get_profiles();

    for (auto &[n, v] : *profiles) {
        GtkWidget *toggle_item = gtk_menu_item_new_with_label((char *)n.c_str());
        g_signal_connect(toggle_item, "activate", G_CALLBACK(on_preset_clicked), strdup((char *)n.c_str()) );
        gtk_menu_shell_append(GTK_MENU_SHELL(*menu), toggle_item);
    }
    GtkWidget *sep_item;
    sep_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(*menu), sep_item);
}

void close_callback(GLFWwindow* window)
{
    printf("Close GLFW Window\n");

    glfwSetWindowShouldClose(window, FALSE);
    glfwHideWindow(window);
}

void open_file_dialog(gpointer data) {
    GtkFileChooserAction action = (bool)data ? GTK_FILE_CHOOSER_ACTION_OPEN :
                                               GTK_FILE_CHOOSER_ACTION_SAVE;
    std::string win_name = (bool)data ? "Open" : "Save";
    std::cout << "--DATA--:" <<  (bool)data << std::endl;
    // Создание файла-диалога
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        (win_name + " File").c_str(),                            // Заголовок окна
        NULL,                                   // Родительское окно (нет родительского)
        action,          // Режим диалога (открытие файла)
        "_Cancel", GTK_RESPONSE_CANCEL,        // Кнопка "Отмена"
        ("_" + win_name).c_str(), GTK_RESPONSE_ACCEPT,          // Кнопка "Открыть"
        NULL                                   // Завершаем аргументы
    );

    // Показать диалог и обработать результат
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        std::cout << "Выбранный файл: " << path << "\n";
    }

    if ((bool)data) {
        conf.parse_config(path);
        conf.print_config();
        std::lock_guard lock(mutex);
        fans = conf.get_fans_settings();
        cpu_or_gpu = conf.get_cpu_or_gpu();

    }
    else {
        conf.write_to_file(path);
    }

    // Уничтожаем диалог
    gtk_widget_destroy(dialog);
}

// Поток для работы с GTK
void gtk_thread_func() {
    gtk_main();  // Запуск цикла GTK
}

std::vector<std::vector<std::pair<unsigned char, unsigned short>>> fan_data;
std::atomic<bool> running = true;
monitoring mon;


void change_speed_thread() {
    using namespace std::chrono_literals;
    float cpu_temp;
    float gpu_temp;
    int pos;
    while (running.load()) {
        mon.update();
        cpu_temp = round((float)mon.get_cpu_temp() / 5.0f) * 5.0f;
        gpu_temp = round((float)mon.get_gpu_temp() / 5.0f) * 5.0f;
        mutex.lock();
        for (size_t d = 0; d < fans->size(); d++) {
            auto fan = &(*fans)[d];
            for (size_t f = 0; f < fan->size(); f++) {
                pos = fan->size() * d + f;
                wrapper.sent_to_fan(d, f + 1, (*cpu_or_gpu)[pos] ?
                                                    //(uint)fans[d][f].find(gpu_temp)->second :
                                                    //(uint)fans[d][f].find(cpu_temp)->second);
                                                    (uint)(*fan)[f].find(gpu_temp)->second :
                                                    (uint)(*fan)[f].find(cpu_temp)->second);
            }
        }
        mutex.unlock();
        sleep(1);
        std::this_thread::sleep_for(1s);
    }
}

void init_gtk(int argc, char* argv[]) {
    gtk_init(NULL, NULL);

    // Создаем индикатор
    AppIndicator *indicator = app_indicator_new("example-indicator", "indicator-messages", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);

    // Устанавливаем иконку
    app_indicator_set_attention_icon_full(indicator, "dialog-information", "New messages");

    // Создаем меню для индикатора
    GtkWidget *menu = gtk_menu_new();

    preset_section(&menu);
    // Добавляем пункт для управления окном
    GtkWidget *toggle_item = gtk_menu_item_new_with_label("Toggle Window");
    g_signal_connect(toggle_item, "activate", G_CALLBACK(on_toggle_window), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), toggle_item);

    // Добавляем пункт для выхода
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);

    gtk_widget_show_all(menu);

    // Устанавливаем меню для индикатора
    app_indicator_set_menu(indicator, GTK_MENU(menu));
}

enum class MODE {
    ALL = 1,
    PER_FAN
};

struct speed {
    int all_speed;
    std::vector<int> per_controller;
    std::vector<std::vector<int>> per_fan;
    MODE mode;
};

void print_plot(std::map<float, float> *cur_fan) {
    if (ImPlot::BeginPlot("Fan Control", ImVec2(-1 , -1), ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
        // Рисуем существующие точки
        ImPlot::SetupAxesLimits(0, 100, 0, 100);
        if (!(*cur_fan).empty()) {
            std::vector<float> xs, ys;
            for (auto&& [x, y] : *cur_fan) {
                xs.push_back(x);
                ys.push_back(y);
            }
            ImPlot::PlotLine("Линия", xs.data(), ys.data(), (*cur_fan).size());
            ImPlot::PlotScatter("Точки", xs.data(), ys.data(), (*cur_fan).size());
        }

        // Обработка кликов мыши
        if (ImPlot::IsPlotHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ImPlotPoint mousePos = ImPlot::GetPlotMousePos(); // Получаем координаты мыши

                std::cout << mousePos.x << " " << mousePos.y << "\n";

                // Округляем координаты до ближайших значений, кратных 5
                float roundedX = round(mousePos.x / 5.0f) * 5.0f;
                float roundedY = round(mousePos.y / 5.0f) * 5.0f;

                // Проверяем, что координаты остаются в диапазоне [0, 100]
                if (roundedX >= 0.0f && roundedX <= 100.0f &&
                    roundedY >= 0.0f && roundedY <= 100.0f) {
                    (*cur_fan)[roundedX] = roundedY; // Добавляем или обновляем точку
                }
            }
            if(ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImPlotPoint mousePos = ImPlot::GetPlotMousePos(); // Получаем координаты мыши

                std::cout << mousePos.x << " " << mousePos.y << "\n";

                // Округляем координаты до ближайших значений, кратных 5
                float roundedX = round(mousePos.x / 5.0f) * 5.0f;
                float roundedY = round(mousePos.y / 5.0f) * 5.0f;

                // Проверяем, что координаты остаются в диапазоне [0, 100]
                if (roundedX >= 0.0f && roundedX <= 100.0f &&
                    roundedY >= 0.0f && roundedY <= 100.0f) {
                    std::erase_if(*cur_fan, [&roundedX](const auto& item) {
                                  auto const& [k, v] = item;
                                  return k == roundedX;
                                  });
                }

            }
        }

        ImPlot::EndPlot();
    }
}

// Main code
int main(int argc, char** argv)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Create window with Vulkan context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+Vulkan example", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }

    speed speed;
    speed.mode = MODE::PER_FAN;

    std::map<float, float> points;
    std::string home_dir(getenv("HOME"));
    path = home_dir + "/.config/config.toml";

    if (std::filesystem::exists(path)) {
        conf.parse_config(path);
        conf.print_config();
    }

    ImVector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    SetupVulkan(extensions);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
    check_vk_result(err);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiWindowFlags_NoBackground;       // Enable Multi-Viewport / Platform Windows
    io.FontGlobalScale = 1.4f;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.RenderPass = wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = g_Allocator;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    glfwSetWindowCloseCallback(window, close_callback);
    glfwHideWindow(window);  // Скрыть окно
    init_gtk(argc, argv);

    std::thread gtk_thread(gtk_thread_func);

    bool set_all = false;
    unsigned char sp;
    unsigned short rpm;
    size_t cnum = wrapper.controllers_num();

    fans = conf.get_fans_settings();
    cpu_or_gpu = conf.get_cpu_or_gpu();

    if (!conf.is_readed()) {
        fan_data.resize(cnum, std::vector<std::pair<unsigned char, unsigned short>>(4));
        fans->resize(cnum, std::vector<std::map<float, float>>(5));
        cpu_or_gpu->resize(20, 0);
        for (auto &&i : *fans) {
            for (auto &&j : i) {
                for (size_t idx = 0; idx <= 100; idx+=10) {
                    j[idx] = 50;
                }
            }
        }
    }

    speed.per_controller.resize(cnum);
    speed.per_fan.resize(cnum, std::vector<int>(5));

    std::thread speed_control(change_speed_thread);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        if (fb_width > 0 && fb_height > 0 && (g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height))
        {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            static float f = 0.0f;
            static int counter = 0;
            static GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(fb_width, fb_height));
            ImGui::Begin("Hello, world!", NULL,  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);                          // Create a window called "Hello, world!" and append into it.

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open")) {
                        need_open_file_dialog = true;
                        is_open = true;
                        g_idle_add((GSourceFunc)open_file_dialog, (gpointer)is_open);
                    }
                    if (ImGui::MenuItem("Save to")) {
                        need_open_file_dialog = true;
                        is_open = false;
                        g_idle_add((GSourceFunc)open_file_dialog, (gpointer)is_open);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Quit")) {
                    on_quit(NULL, NULL);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::Checkbox("Set all", &set_all);
            if (set_all) {
                speed.mode = MODE::ALL;
            }
            else
                speed.mode = MODE::PER_FAN;

            switch (speed.mode) {
                case MODE::ALL:
                    ImGui::SliderInt("All controllers", &speed.all_speed, 20, 100);
                    break;
                case MODE::PER_FAN:
                    if (speed.mode == MODE::PER_FAN) {
                        if ( ImGui::BeginTable("controllers", 5)) {
                            std::lock_guard<std::mutex> lock(mutex);
                            for (size_t i = 0; i < 4; i++) {
                                ImGui::TableNextRow();
                                for (size_t j = 0; j < 3; j++) {
                                    ImGui::TableSetColumnIndex(j);
                                    ImGui::PushID(i * 4 + j);
                                    if (ImGui::Button(std::format("Fan {} {}", i + 1, j + 1).c_str())) {
                                        ImGui::OpenPopup("fctl", ImGuiPopupFlags_AnyPopupLevel);
                                    }
                                    ImGui::SetNextWindowSize(ImVec2(fb_width, fb_height));
                                    if (ImGui::BeginPopupModal("fctl")) {
                                        ImGui::Combo("Monitoring", &(*cpu_or_gpu)[5 * i + j], "CPU\0GPU\0");
                                        ImGui::SameLine();
                                        if (ImGui::Button("Close")) {
                                            ImGui::CloseCurrentPopup();
                                        }
                                        ImPlot::CreateContext();
                                        print_plot(&(*fans)[i][j]);
                                        ImPlot::DestroyContext();
                                        ImGui::EndPopup();

                                    }
                                    ImGui::PopID();
                                }
                            }
                            ImGui::EndTable();
                        }
                    }
                    break;
            }

                if (set_all && ImGui::Button("Apply")) {
                        wrapper.send_to_all_controllers(speed.all_speed);
                }

                if (!set_all && ImGui::Button("Save")) {
                    conf.write_to_file(path);
                }

                ImGui::SameLine();
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::Text("Cpu temp: %d,  %s temp: %d", mon.get_cpu_temp(), mon.get_gpu_name().c_str(), mon.get_gpu_temp());

                mon.update();

                ImGui::End();
            }

        // Rendering
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        if (!is_minimized)
        {
            wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            wd->ClearValue.color.float32[3] = clear_color.w;
            FrameRender(wd, draw_data);
            FramePresent(wd);
        }
    }

    // Cleanup
    err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CleanupVulkanWindow();
    CleanupVulkan();

    running.store(false);

    glfwDestroyWindow(window);
    glfwTerminate();

    gtk_thread.join();
    speed_control.join();

    return 0;
}
