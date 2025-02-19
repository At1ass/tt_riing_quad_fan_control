// test_fan_controller.cpp
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "core/fan_controller.hpp"
#include "core/mediator.hpp"
#include "gmock/gmock.h"
#include "system/config.hpp"

using ::testing::_;

using STATS = std::pair<std::size_t, std::size_t>;
using COLOR = std::array<float, 3>;
using COLOR_BUFFER = std::vector<std::vector<std::array<float, 3>>>;

// Минимальные фейковые реализации для тестирования (если они не определены в
// system/config.hpp) Ниже приведён упрощённый пример; в реальном проекте эти
// классы могут быть более сложными. Мок для sys::HidWrapper с использованием
// Google Mock
class MockHidWrapper : public sys::DeviceController {
   public:
    MOCK_METHOD(STATS, sentToFan,
                (std::size_t controller_idx, std::size_t fan_idx, uint value),
                (override));
    MOCK_METHOD(void, setRGB,
                (std::size_t controller_idx, std::size_t fan_idx, COLOR& colors),
                (override));
    MOCK_METHOD(COLOR_BUFFER, makeColorBuffer,
                (), (override) );   
};

class MockFanMediator : public core::Mediator {
   public:
    MOCK_METHOD(void, notify,
                (EventMessageType type, std::shared_ptr<Message> msg),
                (override));
};
//
// Тестовый фикстур для FanController
//
class FanControllerTest : public ::testing::Test {
   protected:
    std::shared_ptr<sys::System> system;                 // NOLINT
    std::shared_ptr<MockHidWrapper> mockHid;             // NOLINT
    std::shared_ptr<core::FanController> fanController;  // NOLINT
    std::shared_ptr<core::Mediator> mediator;  // Интерфейс медиатора // NOLINT

    void SetUp() override {
        // Создаем простую систему с одним контроллером и одним вентилятором
        system = std::make_shared<sys::System>();
        sys::Controller ctrl;
        ctrl.setIdx(0);
        sys::Fan fan;
        sys::FanSpeedData data;
        sys::FanBezierData bdata;

        fan.setIdx(0);
        fan.setMonitoringMode(sys::MonitoringMode::MONITORING_CPU);
        for (size_t i = MIN_TEMP; i <= MAX_TEMP; i += STEP_TEMP) {
            data.addSpeed(DEFAULT_SPEED);
            data.addTemp(static_cast<float>(i));
        }

        for (size_t i = 0; i < 4; i++) {
            bdata.addControlPoint(std::make_pair(static_cast<double>(i * 10),
                                                 static_cast<double>(i * 10)));
        }

        fan.addData(data);
        fan.addBData(bdata);

        // Инициализируем данные вентилятора: скорость = 55.0
        ctrl.addFan(fan);
        system->addController(ctrl);

        // Создаем моковую HID-обёртку
        mockHid = std::make_shared<MockHidWrapper>();

        // Создаем FanController (конструктор принимает system и hid-обёртку)
        fanController = std::make_shared<core::FanController>(system, mockHid, false);

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
    float temp = 70.0f;  // NOLINT
    // Ожидаем, что для вентилятора с мониторингом CPU будет вызван sentToFan с
    // индексами 0, (0+1) и скоростью 50.0
    ON_CALL(*mockHid, setRGB(_,_,_)).WillByDefault(testing::Return());
    EXPECT_CALL(*mockHid, sentToFan(0, 1, 50)).Times(1);

    fanController->updateCPUfans(temp);
}
