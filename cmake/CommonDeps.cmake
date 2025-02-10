if(NOT TARGET GLFW)
    add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
endif()
target_include_directories(glfw PUBLIC ${GLFW_DIR}/include)

# --- GTK+3 setup ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
# Можно создать импортируемую цель или использовать переменные
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

# --- Libayatana AppIndicator ---
pkg_check_modules(AYATANA REQUIRED ayatana-appindicator3-0.1)
include_directories(${AYATANA_INCLUDE_DIRS})
link_directories(${AYATANA_LIBRARY_DIRS})
add_definitions(${AYATANA_CFLAGS_OTHER})

# --- hidapi ---
# find_package(hidapi REQUIRED)
pkg_check_modules(HIDAPI REQUIRED hidapi-hidraw)
include_directories(${HIDAPI_INCLUDE_DIRS})
link_directories(${HIDAPI_LIBRARY_DIRS})
add_definitions(${HIDAPI_CFLAGS_OTHER})

# --- Vulkan ---
find_package(Vulkan REQUIRED)
# Используем импортируемую цель Vulkan::Vulkan

# --- Dear ImGui ---
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

# --- Project sources ---
# Глобальное обнаружение исходников, можно заменить явным перечислением
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.cpp")
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/implot.cpp"
    "${IMGUI_DIR}/implot_items.cpp"
)

# Создаем интерфейсную библиотеку для заголовков
add_library(HEADERS_INCLUDE INTERFACE)
target_include_directories(HEADERS_INCLUDE INTERFACE ${CMAKE_SOURCE_DIR}/include)
