#include "core/effectsEngine.hpp"

#include <cstdint>

#include "core/logger.hpp"

namespace core {

void EffectsEngine::addEffect(std::unique_ptr<EffectCommand> effect) {
    effects.push_back(std::move(effect));
}

void EffectsEngine::setActiveEffect(std::size_t index) {
    if (index < effects.size() && index >= 0) {
        active_effect = index;
    } else if (index < 0) {
        active_effect = 0;
    } else {
        active_effect = effects.size() - 1;
    }

    core::Logger::log(core::LogLevel::INFO)
        << "Active effect set to index " << active_effect.value() << std::endl;
}

void EffectsEngine::setActiveEffect(std::unique_ptr<EffectCommand> effect) {
    if (active_effect.has_value()) {
        effects[active_effect.value()] = std::move(effect);
        core::Logger::log(core::LogLevel::INFO)
            << "Active effect set" << std::endl;
    }
}

void EffectsEngine::updateActiveEffect(
    std::array<uint8_t, 3> color,
    std::chrono::steady_clock::duration duration) {
    if (active_effect.has_value()) {
        EffectParams params(color[0], color[1], color[2], duration);
        effects[active_effect.value()] =
            std::move(effects[active_effect.value()]->makeNew(params));
    }
}

auto EffectsEngine::hasActiveEffect() const -> bool {
    return active_effect.has_value();
}

void EffectsEngine::resetActiveEffect() { active_effect.reset(); }

auto EffectsEngine::update(std::chrono::steady_clock::duration interval)
    -> std::array<uint8_t, 3> {
    core::Logger::log(core::LogLevel::INFO)
        << "EffectsEngine update" << std::endl;
    if (active_effect.has_value()) {
        auto& choosen_effect = effects[active_effect.value()];
        if (choosen_effect) {
            std::array<uint8_t, 3> result = choosen_effect->execute(interval);
            if (choosen_effect->isFinished()) {
                core::Logger::log(core::LogLevel::INFO)
                    << "Active effect finished" << std::endl;
                active_effect.reset();
            }
            return result;
        }
    }

    return {0, 0, 0};
}

auto EffectsEngine::getEffectCount() const -> std::size_t {
    return effects.size();
}

}  // namespace core
