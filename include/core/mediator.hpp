#ifndef __MEDIATOR_HPP__
#define __MEDIATOR_HPP__

#include <vector>
enum class EventMessageType {
    Initialize,  // Инициализация данных
    UpdateGraph, // Обновление графиков (изменение через GUI)
    UpdateFan    // Обновление данных вентилятора (изменение в системе)
};

struct Message {
    int c_idx;
    int f_idx;
    std::vector<double> t;
    std::vector<double> s;
};

class Mediator {
public:
    virtual ~Mediator() = default;

    void notify(this auto&& self, EventMessageType eventType, Message msg) {
        self.dispatch(eventType, msg);
    }
};

#endif //!__MEDIATOR_HPP_
