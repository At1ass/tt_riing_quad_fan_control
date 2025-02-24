#include "core/commands/fadeCommand.hpp"

#include <memory>

#include "core/effectCommand.hpp"

namespace core {

auto FadeCommand::execute(Duration interval) -> Color {
    if (elapsed >= duration) {
        finished = true;
        current_color = end_color;
    } else {
        double progress = std::chrono::duration<double>(elapsed).count() /
                          std::chrono::duration<double>(duration).count();
        current_color[0] = static_cast<uint8_t>(
            start_color[0] + progress * (end_color[0] - start_color[0]));
        current_color[1] = static_cast<uint8_t>(
            start_color[1] + progress * (end_color[1] - start_color[1]));
        current_color[2] = static_cast<uint8_t>(
            start_color[2] + progress * (end_color[2] - start_color[2]));
    }

    elapsed += interval;

    return current_color;
}

auto FadeCommand::clone() -> std::unique_ptr<EffectCommand> {
    return std::make_unique<FadeCommand>(*this);
}

auto FadeCommand::makeNew(EffectParams const& params)
    -> std::unique_ptr<EffectCommand> {
    return clone();
}

}  // namespace core
