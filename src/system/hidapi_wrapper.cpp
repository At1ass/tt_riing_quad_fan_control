#include "system/hidapi_wrapper.hpp"

#include <hidapi.h>
#include <wchar.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <print>
#include <stdexcept>
#include <utility>

namespace sys {
enum ProtocolType {
    PROTOCOL_SET = 0x32,
    PROTOCOL_GET = 0x33,
    PROTOCOL_INIT = 0xFE
};

enum ProtocolTarget {
    PROTOCOT_FIRMWARE = 0x50,
    PROTOCOL_FAN = 0x51,
    PROTOCOL_LIGHT = 0x52,
};

constexpr std::size_t const START_PRODUCT_ID = 0x232B;
constexpr std::size_t const END_PRODUCT_ID = 0x232E;
constexpr std::size_t const MAX_FAN = 5;

std::string HidWrapper::Device::constructError(std::string const& msg,
                                               wchar_t const* reason) {
    std::string err_msg("Failed hid_open: ");
    std::wstring error(hid_error(NULL));
    err_msg += std::string(error.begin(), error.end());
    return err_msg;
}
void HidWrapper::Device::init() {
    this->dev = hid_open(THERMALTAKE_VID, pid, nullptr);

    if (this->dev == nullptr) {
        throw std::runtime_error(
            constructError("Failed hid_open: ", hid_error(NULL)));
    }
}

void HidWrapper::Device::sendInit() {
    sendRequest(PROTOCOL_INIT, PROTOCOL_GET);
    readResponse();
}

void HidWrapper::Device::showFirmwareVersion() {
    sendRequest(PROTOCOL_GET, PROTOCOT_FIRMWARE);
    std::array<unsigned char, THERMALTAKE_QUAD_PACKET_SIZE> usb_buf =
        readResponse();

    std::println("Firmware version: {}.{}.{}", usb_buf[2], usb_buf[3],
                 usb_buf[4]);
}

void HidWrapper::Device::getFanData(unsigned char port, unsigned char* speed,
                                    uint16_t* rpm) {
    sendRequest(PROTOCOL_GET, PROTOCOT_FIRMWARE);
    std::array<unsigned char, THERMALTAKE_QUAD_PACKET_SIZE> usb_buf =
        readResponse();

    *speed = usb_buf[0x04];
    *rpm = (usb_buf[0x06] << 8) + usb_buf[0x05];  // NOLINT
}

void HidWrapper::Device::sendFan(unsigned char port, unsigned char mode,
                                 unsigned char speed) {
    sendRequest(PROTOCOL_SET, PROTOCOL_FAN, port, mode, speed);
    readResponse();
}

void HidWrapper::Device::showInfo() {
    std::array<wchar_t, HID_MAX_STR> name_string{};
    int ret = 0;

    ret = hid_get_manufacturer_string(dev, name_string.data(), HID_MAX_STR);
    if (ret == -1) {
        throw std::runtime_error(constructError(
            "Failed hid_get_manufacturer_string: ", hid_error(dev)));
    }

    std::wprintf(L"Name: %s\n", name_string.data());  // NOLINT

    ret = hid_get_product_string(dev, name_string.data(), HID_MAX_STR);
    if (ret == -1) {
        throw std::runtime_error(
            constructError("Failed hid_get_product_string: ", hid_error(dev)));
    }

    std::wprintf(L"Prod Name: %s\n", name_string.data());  // NOLINT

    for (size_t fan_index = 0; fan_index < THERMALTAKE_NUM_CHANNELS;
         fan_index++) {
        unsigned char speed = 0;
        uint16_t rpm = 0;

        getFanData(fan_index + 2, &speed, &rpm);

        std::println("---- Fan {}: speed - {}; rpm - {}", fan_index + 1, speed,
                     rpm);
    }
}

void HidWrapper::Device::closeDevice() { hid_close(dev); }

HidWrapper::Device::~Device() {
    if (dev) {
        closeDevice();
    }
}

void HidWrapper::readControllers() {
    struct hid_device_info* devs = nullptr;
    struct hid_device_info* tmp = nullptr;
    std::span<int> pids_span(pids);
    size_t i = 0;

    devs = hid_enumerate(THERMALTAKE_VID, 0);

    if (devs == nullptr) {
        std::string reason("Failed hid_enumerate: ");
        std::wstring error(hid_error(NULL));
        reason += std::string(error.begin(), error.end());
        throw std::runtime_error(reason);
    }

    tmp = devs;
    while (tmp != nullptr) {
        if (tmp->product_id >= START_PRODUCT_ID &&
            tmp->product_id <= END_PRODUCT_ID) {
            pids_span[i++] = tmp->product_id;
        }
        tmp = tmp->next;
    }

    fan_data.resize(pids.size(),
                    std::vector<std::pair<unsigned char, uint16_t>>(MAX_FAN));

    hid_free_enumeration(devs);
}

void HidWrapper::showControllersInfo() {
    for (auto& c : controllers) {
        c.showInfo();
    }
}

void HidWrapper::initControllers() {
    for (auto& p : pids) {
        controllers.emplace_back(p);
    }
    for (auto& c : controllers) {
        c.init();
    }
}

void HidWrapper::getFanData(int controller_id, int fan_id, unsigned char* speed,
                            uint16_t* rpm) {
    controllers[controller_id].getFanData(fan_id, speed, rpm);
}

void HidWrapper::updateFanData() {
    unsigned char speed = 0;
    uint16_t rpm = 0;
    size_t const S = controllers.size();
    failed = false;
    for (size_t c_idx = 0; c_idx < S; c_idx++) {
        for (size_t i = 0; i < MAX_FAN; i++) {
            controllers[c_idx].getFanData(i + 1, &speed, &rpm);
            fan_data[c_idx][i].first = speed;
            fan_data[c_idx][i].second = rpm;
            if (rpm == 0 && i < 3) {
                failed = true;
            }
        }
    }
}

void HidWrapper::sendToController(int controller_id, uint value) {
    for (size_t i = 1; i < 4; i++) {
        controllers[controller_id].sendFan(i, THERMALTAKE_FAN_MODE_FIXED,
                                           value);
    }
}

void HidWrapper::sentToFan(std::size_t controller_id, std::size_t fan_index,
                           uint value) {
    controllers[controller_id].sendFan(fan_index, THERMALTAKE_FAN_MODE_FIXED,
                                       value);
}

void HidWrapper::sendToAllControllers(uint value) {
    for (int i = 0; i < controllers.size(); i++) {
        sendToController(i, value);
    }
}

void HidWrapper::closeControllers() {
    for (auto& c : controllers) {
        c.closeDevice();
    }
}

}  // namespace sys
