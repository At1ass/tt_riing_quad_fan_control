#ifndef __FADE_OUT_COMMAND_HPP__
#define __FADE_OUT_COMMAND_HPP__

#include <array>
#include <chrono>
#include <memory>

#include "core/commands/fadeCommand.hpp"

namespace core {

class FadeOutCommand : public FadeCommand {
   public:
    FadeOutCommand(std::array<uint8_t, 3> const& start,
                   std::chrono::steady_clock::duration duration)
        : FadeCommand(start, std::array<uint8_t, 3>{0, 0, 0}, duration) {}

    std::unique_ptr<EffectCommand> clone() override {
        return std::make_unique<FadeOutCommand>(*this);
    }

    std::unique_ptr<EffectCommand> makeNew(
        EffectParams const& params) override {
        return clone();
    }
};

}  // namespace core

#endif  // !__FADE_OUT_COMMAND_HPP__
