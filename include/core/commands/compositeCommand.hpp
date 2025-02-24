#ifndef __COMPOSITE_COMMAND_HPP__
#define __COMPOSITE_COMMAND_HPP__

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/effectCommand.hpp"

namespace core {

class CompositeCommand : public EffectCommand {
   public:
    CompositeCommand() {}

    void addCommand(std::unique_ptr<EffectCommand> effect);

    std::array<uint8_t, 3> execute(
        std::chrono::steady_clock::duration interval) override;
    bool isFinished() const override;

    std::unique_ptr<EffectCommand> clone() override;
    std::unique_ptr<EffectCommand> makeNew(
        EffectParams const& params) override {
        return clone();
    }

   private:
    std::vector<std::unique_ptr<EffectCommand>> effects;
    std::size_t current_idx{};
};

}  // namespace core

#endif  // !__COMPOSITE_COMMAND_HPP__
