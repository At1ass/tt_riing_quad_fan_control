#include "gui/gtk_tray_manager.hpp"

#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>

#include "core/logger.hpp"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk/gtk.h"
#include "libayatana-appindicator/app-indicator.h"

namespace gui {

GTKTrayManager::GTKTrayManager() {
    gtk_thread = std::jthread([this]() {
        gtk_init(nullptr, nullptr);

        // Создаем индикатор
        indicator = std::shared_ptr<AppIndicator>(
            app_indicator_new("example-indicator", "indicator-messages",
                              APP_INDICATOR_CATEGORY_APPLICATION_STATUS),
            [](AppIndicator* ptr) {});

        app_indicator_set_status(indicator.get(), APP_INDICATOR_STATUS_ACTIVE);

        // Устанавливаем иконку
        app_indicator_set_attention_icon_full(
            indicator.get(), "dialog-information", "New messages");

        // Создаем меню для индикатора
        menu = gtk_menu_new();

        // Добавляем пункт для управления окном
        GtkWidget* toggle_item = gtk_menu_item_new_with_label("Toggle Window");
        g_signal_connect(  // NOLINT
            toggle_item, "activate",
            G_CALLBACK(+[](GtkWidget* /*widget*/, gpointer data) {
                auto* self = static_cast<GTKTrayManager*>(data);
                if (self->callbacks.contains("onToggle")) {
                    for (auto&& callback :
                         self->callbacks["onToggle"]) {
                        callback();
                    }
                }
            }),
            this);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), toggle_item);  // NOLINT

        // Добавляем пункт для выхода
        GtkWidget* quit_item = gtk_menu_item_new_with_label("Quit");
        g_signal_connect(quit_item, "activate",  // NOLINT
                         G_CALLBACK(+[](GtkWidget* /*widget*/, gpointer data) {
                             auto* self = static_cast<GTKTrayManager*>(data);
                             if (self->callbacks.contains("onQuit")) {
                                 for (auto&& callback :
                                      self->callbacks["onQuit"]) {
                                     callback();
                                 }
                             }
                         }),
                         this);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);  // NOLINT

        gtk_widget_show_all(menu);

        // Устанавливаем меню для индикатора
        app_indicator_set_menu(indicator.get(), GTK_MENU(menu));  // NOLINT

        gtk_main();

        running = false;
    });
}

GTKTrayManager::~GTKTrayManager() { stop(); }

void GTKTrayManager::stop() {
    if (running) {
        g_main_context_invoke(
            nullptr,
            [](gpointer) -> gboolean {
                gtk_main_quit();
                return FALSE;
            },
            nullptr);

        if (gtk_thread.joinable()) {
            gtk_thread.join();
        }
    }
}

void GTKTrayManager::cleanup() { gtk_main_quit(); }

void GTKTrayManager::openFileDialog(FileDialogCallback callback) {
    openFileChooserDialog("Open File", GTK_FILE_CHOOSER_ACTION_OPEN,
                          std::move(callback));
}

void GTKTrayManager::saveFileDialog(FileDialogCallback callback) {
    openFileChooserDialog("Save File", GTK_FILE_CHOOSER_ACTION_SAVE,
                          std::move(callback));
}

void GTKTrayManager::openFileChooserDialog(char const* title,
                                           GtkFileChooserAction action,
                                           FileDialogCallback callback) {
    g_idle_add(
        [](gpointer data) -> gboolean {
            std::unique_ptr<std::tuple<char const*, GtkFileChooserAction,
                                       FileDialogCallback>>
                dialog_data(
                    static_cast<std::tuple<char const*, GtkFileChooserAction,
                                           FileDialogCallback>*>(data));
            std::unique_ptr<GtkWidget, std::function<void(GtkWidget*)>> dialog(
                gtk_file_chooser_dialog_new(  // NOLINT
                    std::get<0>(*dialog_data), nullptr,
                    std::get<1>(*dialog_data), "_Cancel", GTK_RESPONSE_CANCEL,
                    (std::get<1>(*dialog_data) == GTK_FILE_CHOOSER_ACTION_SAVE
                         ? "_Save"
                         : "_Open"),
                    GTK_RESPONSE_ACCEPT, nullptr),
                +[](GtkWidget* dialog) { gtk_widget_destroy(dialog); });

            if (std::get<1>(*dialog_data) == GTK_FILE_CHOOSER_ACTION_SAVE) {
                gtk_file_chooser_set_do_overwrite_confirmation(
                    GTK_FILE_CHOOSER(dialog.get()), TRUE);  // NOLINT
            }

            if (gtk_dialog_run(GTK_DIALOG(dialog.get())) ==  // NOLINT
                GTK_RESPONSE_ACCEPT) {
                std::string filename(gtk_file_chooser_get_filename(
                    GTK_FILE_CHOOSER(dialog.get())));  // NOLINT
                if (std::get<2>(*dialog_data)) {
                    std::get<2> (*dialog_data)(filename);  // Вызываем колбэк
                }
            }

            return FALSE;  // Удаляем из очереди g_idle_add
        },
        new std::tuple<char const*, GtkFileChooserAction,
                       FileDialogCallback>(  // NOLINT
            title, action, std::move(callback)));

    core::Logger::log(core::LogLevel::INFO)
        << title << " dialog requested." << std::endl;
}

}  // namespace gui
