// tests/test_fan.cpp
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include "core/logger.hpp"
#include "system/config.hpp"
static std::string syntaxError =
        " saved = ["
        "     ["
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }{ x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 1,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 0,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "     ],"
        " ]";

static std::string incorrectContentCpNum =
        " saved = ["
        "     ["
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 1,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 0,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "     ],"
        " ]";

static std::string incorrectContentCpData =
        " saved = ["
        "     ["
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 60.0, y = 40.0 }, { x = 60, y = 40 }, { x = 100.0, y = 100.0 } ], Monitoring = 1,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 0,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "     ],"
        " ]";

static std::string incorrectCPStructure =
        " saved = ["
        "     ["
        "         { 'Control points' = {  x = 0.0, y = 0.0 }, Monitoring = 1,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 0,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "     ],"
        " ]";

static std::string incorrectMonitoringMode =
        " saved = ["
        "     ["
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 2,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 0,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "     ],"
        " ]";

static std::string incorrectFanData =
        " saved = ["
        "     ["
        "       ["
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 1,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 0,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "       ]"
        "     ],"
        " ]";

static std::string expectedContent =
        " saved = ["
        "     ["
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 1,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "         { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 0,"
        " Speeds = [50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0],"
        " Temps = [ 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 ] "
        "         },"
        "     ],"
        " ]";

class ConfigTestF : public ::testing::Test {
    protected:
        std::pair<std::vector<double>, std::vector<double>> test_fan_data_default;
        std::array<std::pair<double, double>, 4> test_fan_cp_default;

        ConfigTestF(){
            test_fan_data_default =
                std::make_pair<std::vector<double>, std::vector<double>>(
                        { 0.0, 5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0 },
                        { 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0 }
                        );

            test_fan_cp_default =
            {
                std::make_pair<double, double>(0.0, 0.0),
                std::make_pair<double, double>(40.0, 60.0),
                std::make_pair<double, double>(60.0, 40.0),
                std::make_pair<double, double>(100.0, 100.0)
            };
        }
};

TEST(ConfigTest, DefaultSpeedOnTempZero) {
    sys::Fan fan;
    sys::FanSpeedData data = fan.getData();
    sys::FanBezierData bdata = fan.getBData();
    EXPECT_EQ(data.getSpeedForTemp(0), 50.0) << "After create FanSpeedData speed must be 50";
    EXPECT_NEAR(bdata.getSpeedForTemp(0), 0, 0.01) << "After create FanBezierData speed must be 0 for temp 0.";
}

TEST_F(ConfigTestF, ReadConfig) {
 // 1. Создаём временный файл.
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << expectedContent;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
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

        EXPECT_EQ(data, test_fan_data_default) << "Incorrect reading speed and temp";
        EXPECT_EQ(bdata, test_fan_cp_default) << "INcorrect readin control points";
    }

    // 5. Удаляем временный файл.
    std::remove(tempFileName.c_str());
}

TEST(ConfigTest, ReadConfigThrowSyntaxError) {
 // 1. Создаём временный файл.
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << syntaxError;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    EXPECT_ANY_THROW(sys::Config::getInstance().parseConfig(tempFileName));

    std::remove(tempFileName.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectConfigStructure) {
 // 1. Создаём временный файл.
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << incorrectCPStructure;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
        FAIL() << "Expected std::runtime_error exception, but it was not thrown.";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Incorrect config structure");
    } catch (...) {
        FAIL() << "Expected std::runtime_error exception, but it thrown another exception";
    }

    std::remove(tempFileName.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectMonitoringMode) {
 // 1. Создаём временный файл.
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << incorrectMonitoringMode;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
        FAIL() << "Expected std::runtime_error exception, but it was not thrown.";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Incorrect monitoring mode");
    } catch (...) {
        FAIL() << "Expected std::runtime_error exception, but it thrown another exception";
    }

    std::remove(tempFileName.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectFanData) {
 // 1. Создаём временный файл.
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << incorrectFanData;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
        FAIL() << "Expected std::runtime_error exception, but it was not thrown.";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Incorrect fan data");
    } catch (...) {
        FAIL() << "Expected std::runtime_error exception, but it thrown another exception";
    }

    std::remove(tempFileName.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectCPNum) {
 // 1. Создаём временный файл.
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << incorrectContentCpNum;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
        FAIL() << "Expected std::runtime_error exception, but it was not thrown.";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Control points count must be 4");
    } catch (...) {
        FAIL() << "Expected std::runtime_error exception, but it thrown another exception";
    }

    std::remove(tempFileName.c_str());
}

TEST(ConfigTest, ReadConfigThrowIncorrectCPData) {
 // 1. Создаём временный файл.
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 2. Записываем тестовые данные.
    ofs << incorrectContentCpData;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    try {
        std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
        FAIL() << "Expected std::runtime_error exception, but it was not thrown.";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Incorrect cp data");
    } catch (...) {
        FAIL() << "Expected std::runtime_error exception, but it thrown another exception";
    }

    std::remove(tempFileName.c_str());
}

TEST_F(ConfigTestF, DummyFansTest) {
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    sys::Config::getInstance().setControllerNum(3);

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
    auto c = sys->getControllers();

    // 4. Сравниваем полученное содержимое с ожидаемым.
    ASSERT_EQ(c.size(), 3) << "Must be readed three controller settings";

    for (auto && controller : c) {
        auto f = controller.getFans();

        ASSERT_EQ(f.size(), 5) << "Must be readed 2 fans settrings";

        for (size_t i = 0; i < 5; i++) {
            EXPECT_EQ(f[i].getIdx(), i);

            auto data = f[i].getData().getData();
            auto bdata = f[i].getBData().getData();

            EXPECT_EQ(data, test_fan_data_default) << "Incorrect reading speed and temp";
            EXPECT_EQ(bdata, test_fan_cp_default) << "INcorrect readin control points";
        }
    }

    // 5. Удаляем временный файл.
    std::remove(tempFileName.c_str());
}

TEST_F(ConfigTestF, DummyFansTestIfCnumZero) {
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    sys::Config::getInstance().setControllerNum(0);

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
    auto c = sys->getControllers();

    // 4. Сравниваем полученное содержимое с ожидаемым.
    ASSERT_EQ(c.size(), 0) << "Controller settings is empty";

    // 5. Удаляем временный файл.
    std::remove(tempFileName.c_str());
}

TEST_F(ConfigTestF, DummyFansTestIfCnumNotSet) {
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
    auto c = sys->getControllers();

    // 4. Сравниваем полученное содержимое с ожидаемым.
    ASSERT_EQ(c.size(), 0) << "Controller settings is empty";

    // 5. Удаляем временный файл.
    std::remove(tempFileName.c_str());
}

TEST(ConfigTest, PrintConfigTest) {
    std::string tempFileName = "temp_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    ofs << expectedContent;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);

    std::streambuf* oldCoutBuf = std::cout.rdbuf();
    std::ostringstream outputBuffer;
    std::cout.rdbuf(outputBuffer.rdbuf());

    sys::Config::getInstance().printConfig(sys);

    std::cout.rdbuf(oldCoutBuf);

    EXPECT_EQ(outputBuffer.str(), "");
    // 5. Удаляем временный файл.
    std::remove(tempFileName.c_str());
}

static std::string removeNewlines(const std::string& input) {
    std::string output;
    for (char c : input) {
        if (c != '\n' && c != '\r') {
            output.push_back(c);
        }
    }
    return output;
}

TEST(ConfigTest, WriteToFileConfigTest) {
    std::string tempFileName = "temp_test_file.txt";
    std::string tempOutputFileName = "temp_output_test_file.txt";
    std::ofstream ofs(tempFileName);
    ASSERT_TRUE(ofs.is_open()) << "Failed open config file";

    ofs << expectedContent;
    ofs.close();

    // 3. Вызываем функцию чтения файла.
    std::shared_ptr<sys::System> sys = sys::Config::getInstance().parseConfig(tempFileName);
    try {
        sys::Config::getInstance().writeToFile(tempOutputFileName);
    } catch (const std::exception& e) {
        FAIL() << e.what() << std::endl;
    }

    std::ifstream ifs(tempOutputFileName);
    ASSERT_TRUE(ifs.is_open()) << "Failed open config file";
    std::stringstream expectedOutput;

    std::cout << "Hi\n";
    expectedOutput << ifs.rdbuf();
    ifs.close();

    std::string expected = "saved = [    [        { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 1, Speeds = [            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0        ], Temps = [            0.0,            5.0,            10.0,            15.0,            20.0,            25.0,            30.0,            35.0,            40.0,            45.0,            50.0,            55.0,            60.0,            65.0,            70.0,            75.0,            80.0,            85.0,            90.0,            95.0,            100.0        ] },        { 'Control points' = [ { x = 0.0, y = 0.0 }, { x = 40.0, y = 60.0 }, { x = 60.0, y = 40.0 }, { x = 100.0, y = 100.0 } ], Monitoring = 0, Speeds = [            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0,            50.0        ], Temps = [            0.0,            5.0,            10.0,            15.0,            20.0,            25.0,            30.0,            35.0,            40.0,            45.0,            50.0,            55.0,            60.0,            65.0,            70.0,            75.0,            80.0,            85.0,            90.0,            95.0,            100.0        ] }    ]]";

    EXPECT_EQ(removeNewlines(expectedOutput.str()), expected);

    // 5. Удаляем временный файл.
    std::remove(tempFileName.c_str());
}
