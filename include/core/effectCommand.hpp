#ifndef __EFFECT_COMMAND_HPP__
#define __EFFECT_COMMAND_HPP__

#include <chrono>
#include <memory>
namespace core {

struct EffectParams {
    uint8_t r, g, b;
    std::chrono::steady_clock::duration duration;
};

constexpr uint8_t const MIN_CHANNEL_VALUE = 0;
constexpr uint8_t const MAX_CHANNEL_VALUE = 255;

class EffectCommand {
   public:
    virtual ~EffectCommand() = default;
    virtual std::array<uint8_t, 3> execute(
        std::chrono::steady_clock::duration) = 0;
    virtual bool isFinished() const = 0;
    virtual std::unique_ptr<EffectCommand> clone() = 0;
    virtual std::unique_ptr<EffectCommand> makeNew(
        EffectParams const& params) = 0;
};

}  // namespace core

#endif  // !__EFFECT_COMMAND_HPP__
