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
if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "Enabling code coverage flags")
    set(COVERAGE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COVERAGE_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${COVERAGE_CXX_FLAGS}")
endif()

# --- GLFW setup ---
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/3rdparty/glfw")
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
option(BUILD_TESTS "Build unit tests" OFF)

include(cmake/CommonDeps.cmake)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()


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
    # hidapi::hidapi
    ${HIDAPI_LIBRARIES}
    HEADERS_INCLUDE
)

# --- Compiler options based on build type ---
target_compile_options(tt_riing_quad_fan_control PRIVATE
    $<$<CONFIG:Debug>:-O0 -ggdb -fno-rtti -DENABLE_INFO_LOGS>
    $<$<CONFIG:Release>:-O3 -fno-rtti>
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
