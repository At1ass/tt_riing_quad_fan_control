#ifndef __FADE_IN_COMMAND_HPP__
#define __FADE_IN_COMMAND_HPP__

#include <array>
#include <chrono>
#include <memory>

#include "core/commands/fadeCommand.hpp"

namespace core {

class FadeInCommand : public FadeCommand {
   public:
    FadeInCommand(std::array<uint8_t, 3> const& end,
                  std::chrono::steady_clock::duration duration)
        : FadeCommand(std::array<uint8_t, 3>{0, 0, 0}, end, duration) {}

    std::unique_ptr<EffectCommand> clone() override {
        return std::make_unique<FadeInCommand>(*this);
    }

    std::unique_ptr<EffectCommand> makeNew(
        EffectParams const& params) override {
        return clone();
    }
};

}  // namespace core

#endif  // !__FADE_IN_COMMAND_HPP__
