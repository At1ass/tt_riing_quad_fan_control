#include "core/commands/compositeCommand.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include "core/logger.hpp"

namespace core {
void CompositeCommand::addCommand(std::unique_ptr<EffectCommand> effect) {
    effects.push_back(std::move(effect));
}

auto CompositeCommand::execute(std::chrono::steady_clock::duration interval)
    -> std::array<uint8_t, 3> {
    core::Logger::log(core::LogLevel::INFO)
        << "CompositeCommand executed" << std::endl;

    if (effects.empty()) return {0, 0, 0};

    if (current_idx < effects.size()) {
        std::array<uint8_t, 3> result = effects[current_idx]->execute(interval);
        if (effects[current_idx]->isFinished()) {
            core::Logger::log(core::LogLevel::INFO)
                << "Command at index " << current_idx
                << " finished, moving to next command" << std::endl;
            current_idx++;
        }

        return result;
    }

    return {0, 0, 0};
}

auto CompositeCommand::isFinished() const -> bool {
    return current_idx >= effects.size();
}

auto CompositeCommand::clone() -> std::unique_ptr<EffectCommand> {
    auto composite = std::make_unique<CompositeCommand>();
    composite->current_idx = current_idx;
    for (auto const& e : effects) {
        composite->addCommand(e->clone());
    }

    return composite;
}

}  // namespace core
