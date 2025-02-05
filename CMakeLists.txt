cmake_minimum_required(VERSION 3.16)

project(tt_riing_fan_control LANGUAGES C CXX)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Global compile definitions
add_compile_definitions(VK_PROTOTYPES)

# Verbose makefile output (опционально)
set(CMAKE_VERBOSE_MAKEFILE ON)

# --- GLFW setup ---
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/3rdparty/glfw")
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
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
find_package(hidapi REQUIRED)

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

# --- Executable ---
add_executable(tt_riing_quad_fan_control
    ${PROJECT_SOURCES}
    ${SRC_FILES}
    ${IMGUI_SOURCES}
)

target_link_libraries(tt_riing_quad_fan_control
    glfw
    Vulkan::Vulkan
    ${GTK3_LIBRARIES}
    ${AYATANA_LIBRARIES}
    hidapi::hidapi
    HEADERS_INCLUDE
    nvidia-ml
)

# --- Compiler options based on build type ---
target_compile_options(tt_riing_quad_fan_control PRIVATE
    $<$<CONFIG:Debug>:-O0 -ggdb -DENABLE_INFO_LOGS>
    $<$<CONFIG:Release>:-O3>
)

# --- Installation commands ---
# Install executable into bin directory
install(TARGETS tt_riing_quad_fan_control
    RUNTIME DESTINATION bin
)

# Install additional files (LICENSE, README.md)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION share/tt_riing_quad_fan_control
)

# Задаем переменную, указывающую на install_manifest.txt
set(INSTALL_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")

# --- Uninstall target ---
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY
)

add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  COMMENT "Uninstalling the project"
)
