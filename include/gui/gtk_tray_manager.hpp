#ifndef __GTK_TRAY_MANAGER_HPP__
#define __GTK_TRAY_MANAGER_HPP__

#include "glib.h"
#include "gtk/gtk.h"
#include "libayatana-appindicator/app-indicator.h"
#include "core/logger.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

namespace gui {
    class GTKTrayManager {
        public:
            using FileDialogCallback = std::function<void(const std::string&)>;
            using Callback = std::function<void()>;

            GTKTrayManager();
            ~GTKTrayManager();

            void setOnToggleCallback(Callback cb);
            void setOnQuitCallback(Callback cb);
            void stop();
            static void cleanup();
            void openFileDialog(FileDialogCallback callback);
            void saveFileDialog(FileDialogCallback callback);

        private:
            std::shared_ptr<AppIndicator> indicator;
            Callback onToggleCallback = nullptr;
            Callback onQuitCallback = nullptr;
            std::jthread gtk_thread;
            std::atomic<bool> running{true};

            static void openFileChooserDialog(const char* title, GtkFileChooserAction action, FileDialogCallback callback);
    };
}

#endif // !__GTK_TRAY_MANAGER_HPP__
