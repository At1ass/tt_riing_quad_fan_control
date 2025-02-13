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
        GtkWidget* menu = gtk_menu_new();

        // Добавляем пункт для управления окном
        GtkWidget* toggle_item = gtk_menu_item_new_with_label("Toggle Window");
        g_signal_connect(  // NOLINT
            toggle_item, "activate",
            G_CALLBACK(+[](GtkWidget* /*widget*/, gpointer data) {
                auto* self = static_cast<GTKTrayManager*>(data);
                if (self->onToggleCallback) {
                    self->onToggleCallback();
                }
            }),
            this);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), toggle_item);  // NOLINT

        // Добавляем пункт для выхода
        GtkWidget* quit_item = gtk_menu_item_new_with_label("Quit");
        g_signal_connect(quit_item, "activate",  // NOLINT
                         G_CALLBACK(+[](GtkWidget* /*widget*/, gpointer data) {
                             auto* self = static_cast<GTKTrayManager*>(data);
                             if (self->onQuitCallback) {
                                 self->onQuitCallback();
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

void GTKTrayManager::setOnToggleCallback(Callback cb) {
    onToggleCallback = std::move(cb);
}

void GTKTrayManager::setOnQuitCallback(Callback cb) {
    onQuitCallback = std::move(cb);
}

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
            auto* dialogData =
                static_cast<std::tuple<char const*, GtkFileChooserAction,
                                       FileDialogCallback>*>(data);

            // Создаём диалог файлового выбора
            GtkWidget* dialog = gtk_file_chooser_dialog_new(
                std::get<0>(*dialogData), nullptr, std::get<1>(*dialogData),
                "_Cancel", GTK_RESPONSE_CANCEL,
                (std::get<1>(*dialogData) == GTK_FILE_CHOOSER_ACTION_SAVE
                     ? "_Save"
                     : "_Open"),
                GTK_RESPONSE_ACCEPT, nullptr);

            if (std::get<1>(*dialogData) == GTK_FILE_CHOOSER_ACTION_SAVE) {
                gtk_file_chooser_set_do_overwrite_confirmation(
                    GTK_FILE_CHOOSER(dialog), TRUE);
            }

            if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
                char* filename =
                    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
                if (std::get<2>(*dialogData)) {
                    std::get<2> (*dialogData)(filename);  // Вызываем колбэк
                }
                g_free(filename);
            }

            gtk_widget_destroy(dialog);
            delete dialogData;

            return FALSE;  // Удаляем из очереди g_idle_add
        },
        new std::tuple<char const*, GtkFileChooserAction, FileDialogCallback>(
            title, action, std::move(callback)));

    core::Logger::log(core::LogLevel::INFO)
        << title << " dialog requested." << std::endl;
}

}  // namespace gui
