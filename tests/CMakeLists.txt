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

# Добавляем заголовочные файлы проекта
include_directories(${PROJECT_SOURCE_DIR}/../include)
include_directories(${PROJECT_SOURCE_DIR}/../src)

# Если требуется, добавьте исходники проекта для статической линковки
set(PROJECT_SOURCES
    ${PROJECT_SOURCE_DIR}/../src/system/config.cpp
    ${PROJECT_SOURCE_DIR}/../src/core/logger.cpp
    ${PROJECT_SOURCE_DIR}/../src/system/monitoring.cpp
    # ${PROJECT_SOURCE_DIR}/../src/system/
    # ${PROJECT_SOURCE_DIR}/../src/system/monitoring.cpp
    # ${PROJECT_SOURCE_DIR}/../src/system/monitoring.cpp

    # ${PROJECT_SOURCE_DIR}/../src/fan_controller.cpp
    # ${PROJECT_SOURCE_DIR}/../src/temperature_monitor.cpp
    # ${PROJECT_SOURCE_DIR}/../src/config_manager.cpp
)

# Собираем тестовое приложение
set(TEST_SOURCES
    test_config.cpp
    test_monitoring.cpp
)

add_executable(runTests ${TEST_SOURCES} ${PROJECT_SOURCES})
target_link_libraries(runTests gtest gtest_main)

# Добавляем тест в систему CTest
add_test(NAME runTests COMMAND runTests)
