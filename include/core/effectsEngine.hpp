#ifndef __EFFECTS_ENGINE_HPP__
#define __EFFECTS_ENGINE_HPP__

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "core/effectCommand.hpp"

namespace core {

class EffectsEngine {
   public:
    void addEffect(std::unique_ptr<EffectCommand> effect);
    void setActiveEffect(std::size_t index);
    void setActiveEffect(std::unique_ptr<EffectCommand> effect);

    void updateActiveEffect(std::array<uint8_t, 3> color,
                            std::chrono::steady_clock::duration duration);
    bool hasActiveEffect() const;
    void resetActiveEffect();
    std::array<uint8_t, 3> update(
        std::chrono::steady_clock::duration interval);
    std::size_t getEffectCount() const;

   private:
    std::vector<std::unique_ptr<EffectCommand>> effects;
    std::optional<std::size_t> active_effect;
};

}  // namespace core

#endif  // !__EFFECTS_ENGINE_HPP__
