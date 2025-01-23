# Example usage:
#  mkdir build
#  cd build
#  cmake -g "Visual Studio 14 2015" ..

cmake_minimum_required(VERSION 2.8)
project(imgui_example_glfw_vulkan C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -O3")

# GLFW
set(GLFW_DIR 3rdparty/glfw) # Set this to point to an up-to-date GLFW repo
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
include_directories(${GLFW_DIR}/include)

#GTK+3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
# Add other flags to the compiler
add_definitions(${GTK3_CFLAGS_OTHER})

# Libayatana3
pkg_check_modules(PC_AYATANA_APPINDICATOR REQUIRED ayatana-appindicator3-0.1)
include_directories(${PC_AYATANA_APPINDICATOR_INCLUDE_DIRS})
link_directories(${PC_AYATANA_APPINDICATOR_LIBRARY_DIRS})
# Add other flags to the compiler
add_definitions(${PC_AYATANA_APPINDICATOR_CFLAGS_OTHER})

# hidapi
find_package(hidapi REQUIRED)

# my wrapper
#set(WRAPPER_INCLUDE ../include)
file(GLOB SRC CONFIGURE_DEPENDS "src/*.cpp")
#include_directories(${WRAPPER_SRC})
add_library(WRAPPER_INCLUDE INTERFACE)
target_include_directories(WRAPPER_INCLUDE INTERFACE ${CMAKE_SOURCE_DIR}/include)


# Dear ImGui
set(IMGUI_DIR imgui/)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

# Libraries
find_package(Vulkan REQUIRED)
#find_library(VULKAN_LIBRARY
  #NAMES vulkan vulkan-1)
#set(LIBRARIES "glfw;${VULKAN_LIBRARY}")
set(LIBRARIES "glfw;Vulkan::Vulkan")

# Use vulkan headers from glfw:
include_directories(${GLFW_DIR}/deps)

file(GLOB sources *.cpp)

add_executable(example_glfw_vulkan ${sources} ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp ${IMGUI_DIR}/implot.cpp ${IMGUI_DIR}/implot_items.cpp ${SRC})
target_link_libraries(example_glfw_vulkan ${LIBRARIES} ${GTK3_LIBRARIES} ${PC_AYATANA_APPINDICATOR_LIBRARIES} hidapi::hidapi WRAPPER_INCLUDE  nvidia-ml)

