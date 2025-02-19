// tests/test_fan.cpp
#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

#include "core/logger.hpp"
#include "system/config.hpp"

static std::string const SYNTAX_ERROR =
    " saved = ["
    "     ["
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }{ x = 40.0, y = 60.0 "
    "}, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 1,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = "
    "60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "0,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "     ],"
    " ]";

static std::string const INCORRECT_CONTENT_CP_NUM =
    " saved = ["
    "     ["
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 60.0, y = "
    "40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 1,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = "
    "60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "0,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "     ],"
    " ]";

static std::string const INCORRECT_CONTENT_CP_DATA =
    " saved = ["
    "     ["
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 60.0, y = "
    "40.0 }, { x = 60, y = 40 }, { x = 100.0, y = 100.0 } ], Monitoring = 1,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = "
    "60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "0,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "     ],"
    " ]";

static std::string const INCORRECT_CP_STRUCTURE =
    " saved = ["
    "     ["
    "         { 'Control points' = {  x = 0.0, y = 0.0 }, Monitoring = 1,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = "
    "60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "0,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "     ],"
    " ]";

static std::string const INCORRECT_MONITORING_MODE =
    " saved = ["
    "     ["
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = "
    "60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "2,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = "
    "60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "0,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "     ],"
    " ]";

static std::string const INCORRECT_FAN_DATA =
    " saved = ["
    "     ["
    "       ["
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = "
    "60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "1,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = "
    "60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "0,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "       ]"
    "     ],"
    " ]";

static std::string const EXPECTED_CONTENT =
    " saved = ["
    "     ["
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 60.0, y = "
    "40.0 }, { x = 40.0, y = 60.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "1,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 60.0, y = "
    "40.0 }, { x = 40.0, y = 60.0 }, { x = 100.0, y = 100.0 } ], Monitoring = "
    "0,"
    " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, "
    "50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
    " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, "
    "50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
    "         },"
    "     ],"
    " ]";

class ConfigTestF : public ::testing::Test {
   protected:
    std::pair<std::vector<double>, std::vector<double>>
        test_fan_data_default;                                     // NOLINT
    std::array<std::pair<double, double>, 4> test_fan_cp_default;  // NOLINT

    ConfigTestF() {
        test_fan_data_default =
            std::make_pair<std::vector<double>, std::vector<double>>(
                {0.0,  5.0,  10.0, 15.0, 20.0, 25.0, 30.0,
                 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0,
                 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0},
                {50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0,
                 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0,
                 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0});

        test_fan_cp_default = {std::make_pair<double, double>(0.0, 0.0),
                               std::make_pair<double, double>(60.0, 40.0),
                               std::make_pair<double, double>(40.0, 60.0),
                               std::make_pair<double, double>(100.0, 100.0)};
    }
};

TEST(ConfigTest, DefaultSpeedOnTempZero) {
    sys::Fan fan;
    sys::FanSpeedData data = fan.getData();
    sys::FanBezierData bdata = fan.getBData();
    EXPECT_EQ(data.getSpeedForTemp(0), 50.0)
        << "After create FanSpeedData speed must be 50";
    EXPECT_NEAR(bdata.getSpeedForTemp(0), 0, 0.01)
        << "After create FanBezierData speed must be 0 for temp 0.";
}

TEST_F(ConfigTestF, ReadConfig) {
    // 1. Создаём временный файл.
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << EXPECTED_CONTENT;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    // sys::Config::getInstance().setControllerNum(1);
    std::shared_ptr<sys::System> sys =
        sys::Config::getInstance().parseConfig(temp_file_name);
    auto c = sys->getControllers();

    // 4. Сравниваем полученное содержимое с ожидаемым.
    ASSERT_EQ(c.size(), 1) << "Must be readed only one controller settings";
    EXPECT_EQ(c[0].getIdx(), 0) << "Controller ID must be 0";

    auto f = c[0].getFans();

    ASSERT_EQ(f.size(), 2) << "Must be reader 2 fans settrings";

    for (size_t i = 0; i < 2; i++) {
        EXPECT_EQ(f[i].getIdx(), i);

        auto data = f[i].getData().getData();
        auto bdata = f[i].getBData().getData();

        EXPECT_EQ(data, test_fan_data_default)
            << "Incorrect reading speed and temp";
        EXPECT_EQ(bdata, test_fan_cp_default)
            << "Incorrect reading control points";
    }

    // 5. Удаляем временный файл.
    std::remove(temp_file_name.c_str());
}

TEST(ConfigTest, ReadConfigThrowSyntaxError) {
    // 1. Создаём временный файл.
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << SYNTAX_ERROR;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    EXPECT_ANY_THROW(sys::Config::getInstance().parseConfig(temp_file_name));

    std::remove(temp_file_name.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectConfigStructure) {
    // 1. Создаём временный файл.
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << INCORRECT_CP_STRUCTURE;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys =
            sys::Config::getInstance().parseConfig(temp_file_name);
    } catch (std::exception const& e) {
        FAIL() << "Unexpected std::exception. Only toml++ throws exception \n";
    }

    std::remove(temp_file_name.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectMonitoringMode) {
    // 1. Создаём временный файл.
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << INCORRECT_MONITORING_MODE;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys =
            sys::Config::getInstance().parseConfig(temp_file_name);
    } catch (std::runtime_error const& e) {
        FAIL() << "Unexpected std::exception. Only toml++ throws exception \n";
    }

    std::remove(temp_file_name.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectFanData) {
    // 1. Создаём временный файл.
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << INCORRECT_FAN_DATA;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys =
            sys::Config::getInstance().parseConfig(temp_file_name);
    } catch (std::runtime_error const& e) {
        FAIL() << "Unexpected std::exception. Only toml++ throws exception \n";
    }

    std::remove(temp_file_name.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectCPNum) {
    // 1. Создаём временный файл.
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << INCORRECT_CONTENT_CP_NUM;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys =
            sys::Config::getInstance().parseConfig(temp_file_name);
    } catch (std::runtime_error const& e) {
        FAIL() << "Unexpected std::exception. Only toml++ throws exception \n";
    }

    std::remove(temp_file_name.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectCPData) {
    // 1. Создаём временный файл.
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << INCORRECT_CONTENT_CP_DATA;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys =
            sys::Config::getInstance().parseConfig(temp_file_name);
    } catch (std::runtime_error const& e) {
        FAIL() << "Unexpected std::exception. Only toml++ throws exception \n";
    }

    std::remove(temp_file_name.c_str());
}

TEST_F(ConfigTestF, DummyFansTest) {
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    sys::Config::getInstance().setControllerNum(3);

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys =
        sys::Config::getInstance().parseConfig(temp_file_name);
    auto c = sys->getControllers();

    // 4. Сравниваем полученное содержимое с ожидаемым.
    ASSERT_EQ(c.size(), 3) << "Must be readed three controller settings";

    for (auto&& controller : c) {
        auto f = controller.getFans();

        ASSERT_EQ(f.size(), 5) << "Must be readed 2 fans settrings";

        for (size_t i = 0; i < 5; i++) {
            EXPECT_EQ(f[i].getIdx(), i);

            auto data = f[i].getData().getData();
            auto bdata = f[i].getBData().getData();

            EXPECT_EQ(data, test_fan_data_default)
                << "Incorrect reading speed and temp";
            EXPECT_EQ(bdata, test_fan_cp_default)
                << "Incorrect reading control points";
        }
    }

    // 5. Удаляем временный файл.
    std::remove(temp_file_name.c_str());
}

TEST_F(ConfigTestF, DummyFansTestIfCnumZero) {
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    sys::Config::getInstance().setControllerNum(0);

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys =
        sys::Config::getInstance().parseConfig(temp_file_name);
    auto c = sys->getControllers();

    // 4. Сравниваем полученное содержимое с ожидаемым.
    ASSERT_EQ(c.size(), 0) << "Controller settings is empty";

    // 5. Удаляем временный файл.
    std::remove(temp_file_name.c_str());
}

TEST_F(ConfigTestF, DummyFansTestIfCnumNotSet) {
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys =
        sys::Config::getInstance().parseConfig(temp_file_name);
    auto c = sys->getControllers();

    // 4. Сравниваем полученное содержимое с ожидаемым.
    ASSERT_EQ(c.size(), 0) << "Controller settings is empty";

    // 5. Удаляем временный файл.
    std::remove(temp_file_name.c_str());
}

TEST(ConfigTest, PrintConfigTest) {
    std::string temp_file_name = "temp_test_file.txt";
    std::ofstream ofs(temp_file_name);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    ofs << EXPECTED_CONTENT;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys =
        sys::Config::getInstance().parseConfig(temp_file_name);

    std::streambuf* old_cout_buf = std::cout.rdbuf();
    std::ostringstream output_buffer;
    std::cout.rdbuf(output_buffer.rdbuf());

    sys::Config::getInstance().printConfig(sys);

    std::cout.rdbuf(old_cout_buf);

    EXPECT_EQ(output_buffer.str(), "");
    // 5. Удаляем временный файл.
    std::remove(temp_file_name.c_str());
}
