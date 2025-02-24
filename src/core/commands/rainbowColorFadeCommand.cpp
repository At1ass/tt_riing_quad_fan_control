#include "core/commands/rainbowColorFadeCommand.hpp"

#include "core/commands/fadeInCommand.hpp"
#include "core/commands/fadeOutComand.hpp"
#include "core/logger.hpp"

namespace core {

auto RainbowColorFadeCommand::execute(
    std::chrono::steady_clock::duration interval) -> Color {
    core::Logger::log(core::LogLevel::INFO)
        << "RainbowColorCommand executed" << std::endl;
    if (sequence.empty()) {
        return {MIN_CHANNEL_VALUE, MIN_CHANNEL_VALUE, MIN_CHANNEL_VALUE};
    }

    auto& cmd = sequence[current_command_idx];
    auto result = cmd->execute(interval);

    if (cmd->isFinished()) {
        sequence[current_command_idx] = prototype[current_command_idx]->clone();
        current_command_idx = (current_command_idx + 1) % sequence.size();
    }

    return result;
}

auto RainbowColorFadeCommand::clone() -> std::unique_ptr<EffectCommand> {
    return std::make_unique<RainbowColorFadeCommand>(fade_duration);
}

auto RainbowColorFadeCommand::makeNew(EffectParams const& params)
    -> std::unique_ptr<EffectCommand> {
    return std::make_unique<RainbowColorFadeCommand>(params.duration);
}

void RainbowColorFadeCommand::buildSequense() {
    sequence.clear();
    for (std::size_t i = 0; i < rainbow_colors.size(); i++) {
        std::size_t next = (i + 1) % rainbow_colors.size();
        sequence.push_back(
            std::make_unique<FadeOutCommand>(rainbow_colors[i], fade_duration));
        sequence.push_back(std::make_unique<FadeInCommand>(rainbow_colors[next],
                                                           fade_duration));
        prototype.push_back(
            std::make_unique<FadeOutCommand>(rainbow_colors[i], fade_duration));
        prototype.push_back(std::make_unique<FadeInCommand>(
            rainbow_colors[next], fade_duration));
    }
}

}  // namespace core
