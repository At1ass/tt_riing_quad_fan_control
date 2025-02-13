#ifndef __GTK_TRAY_MANAGER_HPP__
#define __GTK_TRAY_MANAGER_HPP__

#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include "core/logger.hpp"
#include "glib.h"
#include "gtk/gtk.h"
#include "libayatana-appindicator/app-indicator.h"

namespace gui {
class GTKTrayManager {
   public:
    using FileDialogCallback = std::function<void(std::string const&)>;
    using Callback = std::function<void()>;

    GTKTrayManager();
    GTKTrayManager(GTKTrayManager const&) = delete;
    GTKTrayManager(GTKTrayManager&&) = delete;
    GTKTrayManager& operator=(GTKTrayManager const&) = delete;
    GTKTrayManager& operator=(GTKTrayManager&&) = delete;
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

    static void openFileChooserDialog(char const* title,
                                      GtkFileChooserAction action,
                                      FileDialogCallback callback);
};

}  // namespace gui
#endif  // !__GTK_TRAY_MANAGER_HPP__
