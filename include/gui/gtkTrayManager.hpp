#ifndef __GTK_TRAY_MANAGER_HPP__
#define __GTK_TRAY_MANAGER_HPP__

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

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

    template <typename... Args>
    void setCallbacks(Args&&... args) {
        static_assert(
            sizeof...(args) % 2 == 0,
            "Callbacks must be provided as pairs of (name, callback)");

        setCallbacksImpl(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void appendMenuItemsWithCallback(Args&&... args) {
        static_assert(sizeof...(args) % 3 == 0,
                      "Callbacks must be provided as (name_item, "
                      "callback_name, callback)");

        appendMenuItemsWithCallbackImpl(std::forward<Args>(args)...);
    }
    void stop();
    static void cleanup();
    void openFileDialog(FileDialogCallback callback);
    void saveFileDialog(FileDialogCallback callback);

   private:
    template <typename Name, typename Callbacks, typename... Rest>
    void setCallbacksImpl(Name&& name, Callbacks&& callback, Rest&&... rest) {
        setSingleCallback(std::forward<Name>(name),
                          std::forward<Callbacks>(callback));
        if constexpr (sizeof...(rest) > 0) {
            setCallbacksImpl(
                std::forward<Rest>(rest)...);
        }
    }

    template <typename CallbackFunc>
    void setSingleCallback(std::string const&& name, CallbackFunc&& callback) {
        if constexpr (std::is_invocable_r_v<void, CallbackFunc>) {
            callbacks[name].emplace_back(
                Callback(std::forward<CallbackFunc>(callback)));
        } else {
            static_assert(AlwaysFalse<CallbackFunc>::value,
                          "Unsupported callback type");
        }
    }

    template <typename NameItems, typename NameCallbacks, typename Callbacks,
              typename... Rest>
    void appendMenuItemsWithCallbackImpl(NameItems&& name_item,
                                         NameCallbacks&& name_callbacks,
                                         Callbacks&& callbacks,
                                         Rest&&... rest) {
        using passing_data =
            std::tuple<GTKTrayManager*, std::string, std::string>;
        this->data_for_calbacks[name_item] = std::make_shared<passing_data>(this, std::forward<NameItems>(name_item), name_callbacks);
        g_idle_add(
            [](gpointer data) -> gboolean {
                passing_data* pass_data = static_cast<passing_data*>(data);

                GtkWidget* appended_item = gtk_menu_item_new_with_label(
                    std::get<1>(*pass_data).c_str());
                g_signal_connect(
                    appended_item, "activate",  // NOLINT
                    G_CALLBACK(+[](GtkWidget* /*widget*/, gpointer data) {
                        passing_data* p_data = static_cast<passing_data*>(data);
                        GTKTrayManager* self = std::get<0>(*p_data);
                        std::string callb = std::get<2>(*p_data);
                        if (self->callbacks.contains(callb)) {
                            for (auto&& c : self->callbacks[callb]) {
                                c();
                            }
                        }
                    }),
                    pass_data);

                gtk_menu_shell_insert(
                    GTK_MENU_SHELL(std::get<0>(*pass_data)->menu),
                    appended_item, std::get<0>(*pass_data)->current_callback_num++);  // NOLINT
                gtk_widget_show_all(std::get<0>(*pass_data)->menu);

                return FALSE;
            },
            this->data_for_calbacks[name_item].get());

        setSingleCallback(std::forward<NameCallbacks>(name_callbacks),
                          std::forward<Callbacks>(callbacks));

        if constexpr (sizeof...(rest) > 0) {
            appendMenuItemsWithCallbackImpl(
                std::forward<Rest>(rest)...);
        }
    }

    template <typename>
    struct AlwaysFalse : std::false_type {};

    GtkWidget* menu = NULL;
    std::shared_ptr<AppIndicator> indicator;
    Callback onToggleCallback = nullptr;
    Callback onQuitCallback = nullptr;
    int current_callback_num = 0;
    std::unordered_map<std::string, std::vector<Callback>> callbacks;
    std::unordered_map<
        std::string,
        std::shared_ptr<std::tuple<GTKTrayManager*, std::string, std::string>>>
        data_for_calbacks;

    std::jthread gtk_thread;
    std::atomic<bool> running{true};

    static void openFileChooserDialog(char const* title,
                                      GtkFileChooserAction action,
                                      FileDialogCallback callback);
};

}  // namespace gui
#endif  // !__GTK_TRAY_MANAGER_HPP__
