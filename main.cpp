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

#include "vulkan.hpp"

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

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static GLFWwindow *window = NULL;
static hid_wrapper wrapper;
static config conf;
static std::atomic<bool> need_get_fan_data = false;
static bool need_open_file_dialog = false;
static bool is_open = false;
static std::atomic<bool> gtk_running = true;
static std::string path;// = "/home/at1ass/.config/config.toml";
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
        /*fans = conf.get_fans_settings();*/

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
        for (auto x : conf.get_next_fan_data_by_temp(cpu_temp, gpu_temp)) {
                wrapper.sent_to_fan(x.first.first, x.first.second + 1, x.second.second);
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
    window = glfwCreateWindow(1280, 720, "Thermaltake Riing Quad fan control", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }

    speed speed;
    speed.mode = MODE::PER_FAN;

    // TODO: create func for this
    std::string home_dir(getenv("HOME"));
    path = home_dir + "/.config/config.toml";

    if (std::filesystem::exists(path)) {
        conf.parse_config(path);
        conf.print_config();
    }
    // !TODO

    ImVector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    /*SetupVulkan(extensions);*/
    Vulkan::SetupVulkan(extensions);

    // Create Window Surface
    Vulkan::CreateVulkanSurface(window);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    Vulkan::SetupVulkanWindow(w, h);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiWindowFlags_NoBackground;       // Enable Multi-Viewport / Platform Windows
    io.FontGlobalScale = 1.4f;

    ImGui::StyleColorsDark();

    Vulkan::ImGuiInitInfo(window);

    glfwSetWindowCloseCallback(window, close_callback);
    glfwHideWindow(window);  // Скрыть окно
    init_gtk(argc, argv);

    std::thread gtk_thread(gtk_thread_func);

    bool set_all = false;
    unsigned char sp;
    unsigned short rpm;
    size_t cnum = wrapper.controllers_num();

    if (!conf.is_readed()) {
        conf.init_dummy_fans(cnum);
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
        Vulkan::CreateOrResizeWindow(fb_width, fb_height);
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        Vulkan::NewFrame();
        ImGui::NewFrame();

        {
            static float f = 0.0f;
            static int counter = 0;
            static GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(fb_width, fb_height));
            ImGui::Begin("Fan ctrl", NULL,  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);                          // Create a window called "Hello, world!" and append into it.

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
                                        ImGui::Combo("Monitoring", conf.get_fan_cpu_or_gpu(i, j), "CPU\0GPU\0");
                                        ImGui::SameLine();
                                        if (ImGui::Button("Close")) {
                                            ImGui::CloseCurrentPopup();
                                        }
                                        ImPlot::CreateContext();
                                        print_plot(conf.get_fan_data(i, j));
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
            Vulkan::Render(draw_data, &clear_color);
        }
    }

    // Cleanup
    Vulkan::ImGuiVulkanShutdown();
    ImGui::DestroyContext();

    Vulkan::CleanUpVulkanWindow();
    Vulkan::CleanUpVulkan();

    running.store(false);

    glfwDestroyWindow(window);
    glfwTerminate();

    gtk_thread.join();
    speed_control.join();

    return 0;
}
