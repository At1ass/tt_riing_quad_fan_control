#ifndef __MEDIATOR_HPP__
#define __MEDIATOR_HPP__

#include <memory>

enum class EventMessageType {
    INITIALIZE,
    UPDATE_GRAPH,
    UPDATE_FAN,
    UPDATE_STATS,
    UPDATE_COLOR,
    UPDATE_EFFECT,
    UPDATE_MONITORING_MODE_UI,
    UPDATE_MONITORING_MODE_FAN
};

struct Message {
    std::size_t c_idx;
    std::size_t f_idx;
};

namespace core {

class Mediator {
   public:
    Mediator(Mediator const&) = default;
    Mediator(Mediator&&) = delete;
    Mediator& operator=(Mediator const&) = default;
    Mediator& operator=(Mediator&&) = delete;
    virtual ~Mediator() = default;
    virtual void notify(EventMessageType event_type,
                        std::shared_ptr<Message> msg) = 0;

   protected:
    Mediator() = default;
};

}  // namespace core
#endif  //!__MEDIATOR_HPP_
