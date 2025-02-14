#include "system/controllers/ttRiingQuadController.hpp"

#include <iterator>
#include <stdexcept>

#include "core/logger.hpp"

namespace sys {

void TTRiingQuadController::sentToFan(std::size_t controller_idx,
                                      std::size_t fan_idx, uint value) {
    hidapi_wrapper->sendRequest<TT_RIING_QUAD_PACKET_SIZE>(
        devices[controller_idx], PROTOCOL_START_BYTE, PROTOCOL_SET,
        PROTOCOL_FAN, fan_idx, PROTOCOL_FAN_MODE_FIXED, value);

    auto ret =
        hidapi_wrapper
            ->readResponse<TT_RIING_QUAD_PACKET_SIZE, TT_RIING_QUAD_TIMEOUT>(
                devices[controller_idx]);

    if (ret[PROTOCOL_STATUS_BYTE] == PROTOCOL_FAIL) {
        core::Logger::log(core::LogLevel::WARNING)
            << "Set fan speed failed: Controller " << controller_idx << " Fan "
            << fan_idx << std::endl;
    }

    hidapi_wrapper->sendRequest<TT_RIING_QUAD_PACKET_SIZE>(
        devices[controller_idx], PROTOCOL_START_BYTE, PROTOCOL_GET,
        PROTOCOL_FAN, fan_idx);

    auto ret_get = hidapi_wrapper->readResponse<TT_RIING_QUAD_PACKET_SIZE, TT_RIING_QUAD_TIMEOUT>(
        devices[controller_idx]);

    if (ret_get[PROTOCOL_STATUS_BYTE] == PROTOCOL_FAIL) {
        core::Logger::log(core::LogLevel::WARNING)
            << "Get fan speed data failed: Controller " << controller_idx
            << " Fan " << fan_idx << std::endl;
    }

    core::Logger::log(core::LogLevel::INFO)
        << "Controller: " << controller_idx << " Fan: " << fan_idx << std::endl;
    core::Logger::log(core::LogLevel::INFO)
        << " Speed: " << ret_get[PROTOCOL_SPEED] << std::endl;
    core::Logger::log(core::LogLevel::INFO)
        << " RPM: "
        << (ret_get[PROTOCOL_RPM_H] << SHIFT) + ret_get[PROTOCOL_RPM_L]
        << std::endl;
}

void TTRiingQuadController::setRGB(std::size_t controller_idx,
                                   std::size_t fan_idx) {
    constexpr unsigned int NUM_COLORS = 54;
    std::array<unsigned char, NUM_COLORS> color_data{};
    std::span<unsigned char> colors_span(color_data);

    for (unsigned int color = 0; color < NUM_COLORS; color++) {
        unsigned int color_idx = color * 3;
        colors_span[color_idx + 0] = 0x00;
        colors_span[color_idx + 1] = 0x00;
        colors_span[color_idx + 2] = 0xFF;
    }
    hidapi_wrapper->sendRequest<TT_RIING_QUAD_PACKET_SIZE>(
        devices[controller_idx], PROTOCOL_START_BYTE, PROTOCOL_SET,
        PROTOCOL_LIGHT, fan_idx, 0x24, color_data);
    auto ret =
        hidapi_wrapper
            ->readResponse<TT_RIING_QUAD_PACKET_SIZE, TT_RIING_QUAD_TIMEOUT>(
                devices[controller_idx]);

    if (ret[PROTOCOL_STATUS_BYTE] == PROTOCOL_FAIL) {
        core::Logger::log(core::LogLevel::WARNING)
            << "Set fan speed failed: Controller " << controller_idx << " Fan "
            << fan_idx << std::endl;
    }
}

void TTRiingQuadController::initControllers() {
    for (auto product_id :
         hidapi_wrapper
             ->getHidEnumerationGeneratorPids<THERMALTAKE_VENDOR_ID>()) {
        if (product_id >= TT_RIING_QUAD_START_PRODUCT_ID &&
            product_id <= TT_RIING_QUAD_END_PRODUCT_ID) {
            devices.push_back(
                hidapi_wrapper->makeDevice<THERMALTAKE_VENDOR_ID>(product_id));
        }
    }
}

void TTRiingQuadController::showControllersInfo() {
    for (auto& dev : devices) {
        hidapi_wrapper->printInfo<TT_RIING_QUAD_NUM_CHANNELS>(dev);
    }
}

void TTRiingQuadController::sendInit(device& dev) {
    hidapi_wrapper->sendRequest<TT_RIING_QUAD_PACKET_SIZE>(
        dev, PROTOCOL_START_BYTE, PROTOCOL_INIT, PROTOCOL_GET);
    auto ret = hidapi_wrapper->readResponse<TT_RIING_QUAD_PACKET_SIZE, TT_RIING_QUAD_TIMEOUT>(dev);

    if (ret[PROTOCOL_STATUS_BYTE] != PROTOCOL_SUCCESS) {
        core::Logger::log(core::LogLevel::ERROR) << "Init failed" << std::endl;
        std::runtime_error("Init request failed");
    }
}

}  // namespace sys
