#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <thread>

// Подключаем заголовок Monitoring
#include "system/monitoring.hpp"

// Создадим фиктивные реализации для тестирования.
class FakeCPUController : public sys::ICPUController {
public:
    FakeCPUController(int initialTemp = 50) : fakeTemp(initialTemp) {}
    bool readCpuTempFile(int &temp) override {
        temp = fakeTemp;
        return true;
    }
    std::string getCPUName() override {
        return "Fake CPU";
    }
private:
    int fakeTemp;
};

class FakeGPUController : public sys::IGPUController {
public:
    FakeGPUController(unsigned int initialTemp = 60) : fakeTemp(initialTemp) {}
    bool readGPUTemp(unsigned int &temp) override {
        temp = fakeTemp;
        return true;
    }
    float getGPUTemp() override{
        return fakeTemp;
    }
    std::string getGPUName() override {
        return "Fake GPU";
    }
private:
    float fakeTemp;
};

// Создадим mock-объект для Observer с помощью Google Mock.
class MockObserver : public core::Observer {
public:
    MOCK_METHOD(void, onEvent, (const core::Event&), (override));
};

// Тест, проверяющий, что Monitoring уведомляет наблюдателя при изменении температуры.
TEST(MonitoringTest, NotifiesObserverOnTemperatureChange) {
    using namespace std::chrono_literals;

    // Создаем фиктивные контроллеры с фиксированными температурами.
    auto cpuController = std::make_unique<FakeCPUController>(50);
    auto gpuController = std::make_unique<FakeGPUController>(60);

    // Создаем mock-объект наблюдателя.
    auto observer = std::make_shared<MockObserver>();

    // Ожидаем, что наблюдатель получит уведомления с типами CPU_TEMP_CHANGED и GPU_TEMP_CHANGED.
    EXPECT_CALL(*observer, onEvent(::testing::Field(&core::Event::type, core::EventType::CPU_TEMP_CHANGED)))
        .Times(::testing::AtLeast(1));
    EXPECT_CALL(*observer, onEvent(::testing::Field(&core::Event::type, core::EventType::GPU_TEMP_CHANGED)))
        .Times(::testing::AtLeast(1));

    // Создаем Monitoring, передавая фиктивные контроллеры.
    auto monitoring = std::make_unique<sys::Monitoring>(std::move(cpuController), std::move(gpuController), std::chrono::milliseconds(100));
    monitoring->addObserver(observer);

    // Ждем, чтобы фоновый поток успел выполнить хотя бы один цикл опроса (в monitoringLoop используется sleep 1 секунда).
    std::this_thread::sleep_for(1500ms);

    // Завершаем работу Monitoring (в деструкторе вызывается stop()).
    monitoring.reset();
}

// Тест, проверяющий корректное возвращение имен устройств.
TEST(MonitoringTest, ReturnsCorrectNames) {
    auto cpuController = std::make_unique<FakeCPUController>(50);
    auto gpuController = std::make_unique<FakeGPUController>(60);

    auto monitoring = std::make_unique<sys::Monitoring>(std::move(cpuController), std::move(gpuController));

    EXPECT_EQ(monitoring->getCpuName(), "Fake CPU");
    EXPECT_EQ(monitoring->getGpuName(), "Fake GPU");

    monitoring.reset();
}
