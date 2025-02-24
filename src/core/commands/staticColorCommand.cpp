#include "core/commands/staticColorCommand.hpp"
#include "core/logger.hpp"

namespace core {

auto StaticColorCommand::execute(std::chrono::steady_clock::duration interval)
    -> Color {
    core::Logger::log(core::LogLevel::INFO)
        << "StaticColorCommand executed" << std::endl;
    elapsed += interval;
    core::Logger::log(core::LogLevel::INFO)
        << "Result:" << r << " " << g << " " << b << std::endl;
    return {r, g, b};
}

auto StaticColorCommand::clone() -> std::unique_ptr<EffectCommand> {
    return std::make_unique<StaticColorCommand>(r, g, b, duration);
}

auto StaticColorCommand::makeNew(EffectParams const& params)
    -> std::unique_ptr<EffectCommand> {
    return std::make_unique<StaticColorCommand>(params.r, params.g, params.b,
                                                params.duration);
}

auto StaticColorCommand::makeStaticColorCommand(
    uint8_t r, uint8_t g, uint8_t b,
    std::chrono::steady_clock::duration duration)
    -> std::unique_ptr<EffectCommand> {
    return std::make_unique<StaticColorCommand>(r, g, b, duration);
}

}  // namespace core
