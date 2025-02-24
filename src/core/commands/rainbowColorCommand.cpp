#include "core/commands/rainbowColorCommand.hpp"

#include <chrono>

#include "core/commands/fadeCommand.hpp"
#include "core/effectCommand.hpp"
#include "core/logger.hpp"

namespace core {

auto RainbowColorCommand::execute(std::chrono::steady_clock::duration interval)
    -> Color {
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

auto RainbowColorCommand::clone() -> std::unique_ptr<EffectCommand> {
    return std::make_unique<RainbowColorCommand>(fade_duration);
}

auto RainbowColorCommand::makeNew(EffectParams const& params)
    -> std::unique_ptr<EffectCommand> {
    return std::make_unique<RainbowColorCommand>(params.duration);
}

void RainbowColorCommand::buildSequense() {
    sequence.clear();
    for (std::size_t i = 0; i < rainbow_colors.size(); i++) {
        std::size_t next = (i + 1) % rainbow_colors.size();
        sequence.push_back(std::make_unique<FadeCommand>(
            rainbow_colors[i], rainbow_colors[next], fade_duration));
        prototype.push_back(std::make_unique<FadeCommand>(
            rainbow_colors[i], rainbow_colors[next], fade_duration));
    }
}

}  // namespace core
