#ifndef __MEDIATOR_HPP__
#define __MEDIATOR_HPP__

#include <memory>
#include <vector>
enum class EventMessageType {
    Initialize,  // Инициализация данных
    UpdateGraph, // Обновление графиков (изменение через GUI)
    UpdateFan,    // Обновление данных вентилятора (изменение в системе)
    UpdateMonitoringModeUi,
    UpdateMonitoringModeFan
};

struct Message {
    int c_idx;
    int f_idx;
};

class Mediator {
public:
    virtual ~Mediator() = default;

    void notify(this auto&& self, EventMessageType eventType, std::shared_ptr<Message> msg) {
        self.dispatch(eventType, msg);
    }
};

#endif //!__MEDIATOR_HPP_
