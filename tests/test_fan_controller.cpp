// test_fan_controller.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <utility>
#include "core/fan_controller.hpp"
#include "core/mediator.hpp"
#include "system/config.hpp"

using ::testing::_;

// Минимальные фейковые реализации для тестирования (если они не определены в system/config.hpp)
// Ниже приведён упрощённый пример; в реальном проекте эти классы могут быть более сложными.
// Мок для sys::HidWrapper с использованием Google Mock
class MockHidWrapper : public sys::IHidWrapper {
public:
    MOCK_METHOD(void, sentToFan, (int controllerIdx, int fanIdx, uint value), (override));
};

class MockFanMediator : public core::Mediator {
public:
    MOCK_METHOD(void, notify, (EventMessageType type, std::shared_ptr<Message> msg), (override));
};
//
// Тестовый фикстур для FanController
//
class FanControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<sys::System> system;
    std::shared_ptr<MockHidWrapper> mockHid;
    std::shared_ptr<core::FanController> fanController;
    std::shared_ptr<core::Mediator> mediator; // Интерфейс медиатора

    void SetUp() override {
        // Создаем простую систему с одним контроллером и одним вентилятором
        system = std::make_shared<sys::System>();
        sys::Controller ctrl;
        ctrl.setIdx(0);
        sys::Fan fan;
        sys::FanSpeedData data;
        sys::FanBezierData bdata;

        fan.setIdx(0);
        fan.setMonitoringMode(sys::MONITORING_MODE::MONITORING_CPU);
        for (size_t i = 0; i <= 100; i+=5) {
            data.addSpeed(50);
            data.addTemp(static_cast<float>(i));
        }

        for (size_t i = 0; i < 4; i++) {
            bdata.addControlPoint(std::make_pair(static_cast<double>(i * 10), static_cast<double>(i * 10)));
        }

        fan.addData(data);
        fan.addBData(bdata);

        // Инициализируем данные вентилятора: скорость = 55.0
        ctrl.addFan(fan);
        system->addController(ctrl);

        // Создаем моковую HID-обёртку
        mockHid = std::make_shared<MockHidWrapper>();

        // Создаем FanController (конструктор принимает system и hid-обёртку)
        fanController = std::make_shared<core::FanController>(system, mockHid);

        // Создаем моковый медиатор
        mediator = std::make_shared<MockFanMediator>();
        // setMediator теперь принимает указатель на IFanMediator
        fanController->setMediator(mediator);
    }
};

//
// Тесты
//

// Проверяем, что updateCPUfans вызывает метод sentToFan через HID-обёртку
TEST_F(FanControllerTest, UpdateCPUfansCallsHidWrapperAndNotifiesMediator) {
    float temp = 70.0f;
    // Ожидаем, что для вентилятора с мониторингом CPU будет вызван sentToFan с индексами 0, (0+1) и скоростью 50.0
    EXPECT_CALL(*mockHid, sentToFan(0, 1, 50)).Times(1);

    fanController->updateCPUfans(temp);
}

// Проверяем, что метод updateFanData обновляет данные вентилятора и уведомляет медиатор
TEST_F(FanControllerTest, UpdateFanDataUpdatesSystemAndNotifiesMediator) {
    std::vector<double> temperatures = {65.0, 70.0};
    std::vector<double> speeds = {50.0, 60.0};

    EXPECT_CALL(*static_cast<MockFanMediator*>(mediator.get()),
                notify(::testing::Eq(EventMessageType::UpdateFan), ::testing::_))
        .Times(1);

    fanController->updateFanData(0, 0, temperatures, speeds);

    auto &fan = system->getControllers()[0].getFans()[0];
    auto data = fan.getData().getData();
    EXPECT_EQ(data.first, temperatures);
    EXPECT_EQ(data.second, speeds);
}

// Проверяем, что updateFanMonitoringMode обновляет режим вентилятора и уведомляет медиатор
TEST_F(FanControllerTest, UpdateFanMonitoringModeUpdatesAndNotifiesMediator) {
    // Изначально мониторинг у вентилятора = MONITORING_CPU
    EXPECT_EQ(system->getControllers()[0].getFans()[0].getMonitoringMode(),
              sys::MONITORING_MODE::MONITORING_CPU);

    EXPECT_CALL(*static_cast<MockFanMediator*>(mediator.get()),
                notify(::testing::Eq(EventMessageType::UpdateMonitoringModeFan), ::testing::_))
        .Times(1);

    // Обновляем режим: передаем 1 (MONITORING_GPU)
    fanController->updateFanMonitoringMode(0, 0, 1);

    EXPECT_EQ(system->getControllers()[0].getFans()[0].getMonitoringMode(),
              sys::MONITORING_MODE::MONITORING_GPU);
}
