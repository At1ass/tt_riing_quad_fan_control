#include "system/controllers/ttRiingQuadController.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "core/logger.hpp"

namespace sys {

std::pair<std::size_t, std::size_t> TTRiingQuadController::sentToFan(
    std::size_t controller_idx, std::size_t fan_idx, uint value) {
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

    auto ret_get =
        hidapi_wrapper
            ->readResponse<TT_RIING_QUAD_PACKET_SIZE, TT_RIING_QUAD_TIMEOUT>(
                devices[controller_idx]);

    if (ret_get[PROTOCOL_STATUS_BYTE] == PROTOCOL_FAIL) {
        core::Logger::log(core::LogLevel::WARNING)
            << "Get fan speed data failed: Controller " << controller_idx
            << " Fan " << fan_idx << std::endl;
    }

    std::size_t speed = ret_get[PROTOCOL_SPEED];
    std::size_t rpm =
        (ret_get[PROTOCOL_RPM_H] << SHIFT) + ret_get[PROTOCOL_RPM_L];
    core::Logger::log(core::LogLevel::INFO)
        << "Controller: " << controller_idx << " Fan: " << fan_idx << std::endl;
    core::Logger::log(core::LogLevel::INFO)
        << " Speed: " << ret_get[PROTOCOL_SPEED] << std::endl;
    core::Logger::log(core::LogLevel::INFO)
        << " RPM: "
        << (ret_get[PROTOCOL_RPM_H] << SHIFT) + ret_get[PROTOCOL_RPM_L]
        << std::endl;

    return {speed, rpm};
}

void TTRiingQuadController::setRGB(std::size_t controller_idx,
                                   std::size_t fan_idx,
                                   std::array<uint8_t, 3>& colors) {
    constexpr unsigned int NUM_COLORS = 54;

    std::array<unsigned char, 3 * NUM_COLORS> color_data{};
    std::span<unsigned char> colors_span(color_data);

    for (unsigned int color = 0; color < NUM_COLORS; color++) {
        unsigned int color_idx = color * 3;
        colors_span[color_idx + 0] = colors[0];
        colors_span[color_idx + 1] = colors[1];
        colors_span[color_idx + 2] = colors[2];
    }
    hidapi_wrapper->sendRequest<TT_RIING_QUAD_PACKET_SIZE>(
        devices[controller_idx], PROTOCOL_START_BYTE, PROTOCOL_SET,
        PROTOCOL_LIGHT, fan_idx, PROTOCOL_PER_LED, color_data);
    auto ret =
        hidapi_wrapper
            ->readResponse<TT_RIING_QUAD_PACKET_SIZE, TT_RIING_QUAD_TIMEOUT>(
                devices[controller_idx]);

    if (ret[PROTOCOL_STATUS_BYTE] == PROTOCOL_FAIL) {
        core::Logger::log(core::LogLevel::WARNING)
            << "Set fan color failed: Controller " << controller_idx << " Fan "
            << fan_idx << std::endl;
    }
}

std::vector<std::vector<std::array<uint8_t, 3>>>
TTRiingQuadController::makeColorBuffer() {
    std::vector<std::vector<std::array<uint8_t, 3>>> color_buffer;
    size_t cnum = controllersNum();

    for (size_t i = 0; i < cnum; i++) {
        std::vector<std::array<uint8_t, 3>> fan;
        for (size_t j = 0; j < TT_RIING_QUAD_NUM_CHANNELS; j++) {
            fan.push_back(std::array<uint8_t, 3>{0x00, 0x00, 0xFF});
        }
        color_buffer.push_back(fan);
    }

    return color_buffer;
}

void TTRiingQuadController::initControllers() {
    for (auto device_path :
         hidapi_wrapper->getHidEnumerationGeneratorPaths<
             THERMALTAKE_VENDOR_ID, TT_RIING_QUAD_PRODUCT_IDS_NUM>(
             TT_RIING_QUAD_PRODUCT_IDS)) {
        devices.push_back(
            hidapi_wrapper->makeDevice<THERMALTAKE_VENDOR_ID>(device_path));
    }

    for (auto& dev : devices) {
        sendInit(dev);
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
    auto ret = hidapi_wrapper->readResponse<TT_RIING_QUAD_PACKET_SIZE,
                                            TT_RIING_QUAD_TIMEOUT>(dev);

    if (ret[PROTOCOL_STATUS_BYTE] != PROTOCOL_SUCCESS) {
        core::Logger::log(core::LogLevel::ERROR) << "Init failed" << std::endl;
        std::runtime_error("Init request failed");
    }

    core::Logger::log(core::LogLevel::INFO) << "Init success" << std::endl;
}

unsigned int TTRiingQuadController::convertChannel(float val) {
    return static_cast<unsigned char>(val * COLOR_MULTIPLIER);
}

}  // namespace sys
