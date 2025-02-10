cmake_minimum_required(VERSION 3.10)
project(tt_riing_quad_fan_control_tests)

set(CMAKE_CXX_STANDARD 23)
if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "Enabling code coverage flags")
    set(COVERAGE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COVERAGE_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${COVERAGE_CXX_FLAGS}")
endif()
# Включаем тестирование
enable_testing()

# Добавляем Google Test как поддиректорию.
# Если вы используете подмодуль, путь должен указывать на каталог, куда был клонирован googletest.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/googletest ${CMAKE_CURRENT_BINARY_DIR}/googletest)

# Собираем тестовое приложение
set(TEST_SOURCES
    test_config.cpp
    test_monitoring.cpp
)

add_executable(runTests
    ${TEST_SOURCES}
    ${SRC_FILES}
    ${IMGUI_SOURCES}
)
target_link_libraries(runTests
    gtest
    gtest_main
    gmock
    glfw
    Vulkan::Vulkan
    ${GTK3_LIBRARIES}
    ${AYATANA_LIBRARIES}
    ${HIDAPI_LIBRARIES}
    HEADERS_INCLUDE
)

# Добавляем тест в систему CTest
add_test(NAME runTests COMMAND runTests)
