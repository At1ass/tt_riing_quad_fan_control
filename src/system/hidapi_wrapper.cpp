#include "system/hidapi_wrapper.hpp"
#include <cstring>
#include <hidapi.h>
#include <print>
#include <stdexcept>
#include <utility>

namespace sys {
    enum PROTOCOL_TYPE{
        PROTOCOL_SET = 0x32,
        PROTOCOL_GET = 0x33
    };

    enum PROTOCOL_TARGET {
        PROTOCOT_FIRMWARE = 0x50,
        PROTOCOL_FAN = 0x51,
        PROTOCOL_LIGHT = 0x52,
    };



    std::string HidWrapper::Device::construct_error(const std::string& msg, const wchar_t* reason) {
        std::string err_msg("Failed hid_open: ");
        std::wstring error(hid_error(NULL));
        err_msg += std::string(error.begin(), error.end());
        return err_msg;
    }
    void
        HidWrapper::Device::init()
        {
            this->dev = hid_open(THERMALTAKE_VID, pid, nullptr);

            if (this->dev == nullptr) {
                throw std::runtime_error(construct_error("Failed hid_open: ", hid_error(NULL)));
            }

        }

    void
        HidWrapper::Device::sendInit()
        {
            send_request(0xFE, PROTOCOL_GET);
            read_response();
        }

    void
        HidWrapper::Device::showFirmwareVersion()
        {
            send_request(PROTOCOL_GET, PROTOCOT_FIRMWARE);
            std::array<unsigned char, THERMALTAKE_QUAD_PACKET_SIZE> usb_buf = read_response();

            std::println("Firmware version: {}.{}.{}", usb_buf[2], usb_buf[3], usb_buf[4]);
        }

    void
        HidWrapper::Device::getFanData ( unsigned char       port,
                unsigned char *     speed,
                unsigned short *    rpm)
        {
            send_request(PROTOCOL_GET, PROTOCOT_FIRMWARE);
            std::array<unsigned char, THERMALTAKE_QUAD_PACKET_SIZE> usb_buf = read_response();

            *speed = usb_buf[0x04];
            *rpm   = (usb_buf[0x06] << 8) + usb_buf[0x05];
        }

    void
        HidWrapper::Device::sendFan ( unsigned char       port,
                unsigned char       mode,
                unsigned char       speed)
        {
            send_request(PROTOCOL_SET, PROTOCOL_FAN, port, mode, speed);
            read_response();
        }

    void
        HidWrapper::Device::showInfo() {
            wchar_t name_string[HID_MAX_STR];
            int ret;

            ret = hid_get_manufacturer_string(dev, name_string, HID_MAX_STR);
            if ( ret == -1) {
                throw std::runtime_error(construct_error("Failed hid_get_manufacturer_string: ", hid_error(dev)));
            }

            printf("Name %ls\n", name_string);
            std::wprintf(L"Name: %s\n", name_string);

            ret = hid_get_product_string(dev, name_string, HID_MAX_STR);
            if ( ret == -1) {
                throw std::runtime_error(construct_error("Failed hid_get_product_string: ", hid_error(dev)));
            }

            std::wprintf(L"Prod Name: %s\n", name_string);
            printf("Prod Name: %ls\n", name_string);

            for (size_t fan_index = 0; fan_index < THERMALTAKE_NUM_CHANNELS; fan_index++) {
                unsigned char speed = 0;
                unsigned short rpm = 0;

                getFanData(fan_index + 2, &speed, &rpm);

                std::println("---- Fan {}: speed - {}; rpm - {}",fan_index + 1, speed, rpm);
            }
        }

    void HidWrapper::Device::closeDevice()
    {
        hid_close(dev);
    }

    void HidWrapper::readControllers() {
        struct hid_device_info *devs;
        struct hid_device_info *tmp;
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
            if (tmp->product_id >= 0x232b && tmp->product_id <= 0x232b + 3) {
                pids[i++] = tmp->product_id;
            }
            tmp = tmp->next;
        }

        fan_data.resize(pids.size(), std::vector<std::pair<unsigned char, unsigned short>>(5));

        hid_free_enumeration(devs);
    }

    void HidWrapper::showControllersInfo() {
        for (auto &c : controllers) {
            c.showInfo();
        }
    }

    void HidWrapper::initControllers() {
        for (auto &p : pids) {
            controllers.emplace_back(p);
        }
        for (auto &c : controllers) {
            c.init();
        }
    }

    void HidWrapper::getFanData(int controller_id, int fan_id, unsigned char *speed, unsigned short *rpm) {
        controllers[controller_id].getFanData(fan_id, speed, rpm);
    }

    void HidWrapper::updateFanData() {
        unsigned char speed = 0;
        unsigned short rpm = 0;
        size_t const s = controllers.size();
        failed = false;
        for (size_t c_idx = 0; c_idx < s; c_idx++) {
            for (size_t i = 0; i < 5; i++) {
                controllers[c_idx].getFanData(i + 1, &speed, &rpm);
                fan_data[c_idx][i].first = speed;
                fan_data[c_idx][i].second = rpm;
                if (rpm == 0 && i < 3) { failed = true;
                }
            }
        }
    }

    void HidWrapper::sendToController(int controller_id, uint value) {
        for (size_t i = 1; i < 4; i++) {
            controllers[controller_id].sendFan(i, THERMALTAKE_FAN_MODE_FIXED, value);
        }
    }

    void HidWrapper::sentToFan(int controller_id, int fan_index, uint value) {
        controllers[controller_id].sendFan(fan_index, THERMALTAKE_FAN_MODE_FIXED, value);
    }

    void HidWrapper::sendToAllControllers(uint value) {
        for (int i = 0; i < controllers.size(); i++) {
            sendToController(i, value);
        }
    }

    void HidWrapper::closeControllers() {
        for (auto &c : controllers) {
            c.closeDevice();
        }
    }
}
