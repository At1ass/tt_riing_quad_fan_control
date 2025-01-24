#include "system/hidapi_wrapper.hpp"
#include <cstring>
#include <print>
#include <utility>

namespace sys {
    void
        HidWrapper::Device::init()
        {
            this->dev = hid_open(THERMALTAKE_VID, pid, nullptr);
        }

    void
        HidWrapper::Device::sendInit()
        {
            unsigned char usb_buf[THERMALTAKE_QUAD_PACKET_SIZE];

            /*-----------------------------------------------------*\
              | Zero out buffer                                       |
              \*-----------------------------------------------------*/
            memset(usb_buf, 0x00, sizeof(usb_buf));

            /*-----------------------------------------------------*\
              | Set up Init packet                                    |
              \*-----------------------------------------------------*/
            usb_buf[0x00]   = 0x00;
            usb_buf[0x01]   = 0xFE;
            usb_buf[0x02]   = 0x33;

            /*-----------------------------------------------------*\
              | Send packet                                           |
              \*-----------------------------------------------------*/
            hid_write(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE);
            hid_read_timeout(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE, THERMALTAKE_QUAD_INTERRUPT_TIMEOUT);
        }

    void
        HidWrapper::Device::showFirmwareVersion()
        {
            unsigned char usb_buf[THERMALTAKE_QUAD_PACKET_SIZE];

            /*-----------------------------------------------------*\
              | Zero out buffer                                       |
              \*-----------------------------------------------------*/
            memset(usb_buf, 0x00, sizeof(usb_buf));

            /*-----------------------------------------------------*\
              | Set up Get Firmware Version packet                    |
              \*-----------------------------------------------------*/
            usb_buf[0x00]   = 0x00;
            usb_buf[0x01]   = 0x33;
            usb_buf[0x02]   = 0x50;

            /*-----------------------------------------------------*\
              | Send packet                                           |
              \*-----------------------------------------------------*/
            hid_write(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE);
            hid_read_timeout(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE, THERMALTAKE_QUAD_INTERRUPT_TIMEOUT);

            std::println("Firmware version: {}.{}.{}", usb_buf[2], usb_buf[3], usb_buf[4]);
        }

    void
        HidWrapper::Device::getFanData ( unsigned char       port,
                unsigned char *     speed,
                unsigned short *    rpm)
        {
            unsigned char usb_buf[THERMALTAKE_QUAD_PACKET_SIZE];
            int ret = 0;

            /*-----------------------------------------------------*\
              | Zero out buffer                                       |
              \*-----------------------------------------------------*/
            memset(usb_buf, 0x00, sizeof(usb_buf));

            /*-----------------------------------------------------*\
              | Set up Get Fan Data packet                            |
              \*-----------------------------------------------------*/
            usb_buf[0x00]   = 0x00;
            usb_buf[0x01]   = 0x33;
            usb_buf[0x02]   = 0x51;
            usb_buf[0x03]   = port;

            /*-----------------------------------------------------*\
              | Send packet                                           |
              \*-----------------------------------------------------*/
            ret = hid_write(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE);
            ret = hid_read_timeout(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE, THERMALTAKE_QUAD_INTERRUPT_TIMEOUT);

            *speed = usb_buf[0x04];
            *rpm   = (usb_buf[0x06] << 8) + usb_buf[0x05];
        }

    void
        HidWrapper::Device::sendFan ( unsigned char       port,
                unsigned char       mode,
                unsigned char       speed)
        {
            unsigned char usb_buf[THERMALTAKE_QUAD_PACKET_SIZE];

            /*-----------------------------------------------------*\
              | Zero out buffer                                       |
              \*-----------------------------------------------------*/
            memset(usb_buf, 0x00, sizeof(usb_buf));

            /*-----------------------------------------------------*\
              | Set up RGB packet                                     |
              \*-----------------------------------------------------*/
            usb_buf[0x00]   = 0x00;
            usb_buf[0x01]   = 0x32;
            usb_buf[0x02]   = 0x51;
            usb_buf[0x03]   = port;
            usb_buf[0x04]   = mode;
            usb_buf[0x05]   = speed;

            /*-----------------------------------------------------*\
              | Send packet                                           |
              \*-----------------------------------------------------*/
            hid_write(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE);
            hid_read_timeout(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE, THERMALTAKE_QUAD_INTERRUPT_TIMEOUT);
        }

    void
        HidWrapper::Device::showInfo() {
            wchar_t name_string[HID_MAX_STR];

            hid_get_manufacturer_string(dev, name_string, HID_MAX_STR);

            std::wprintf(L"Name: %s\n", name_string);

            hid_get_product_string(dev, name_string, HID_MAX_STR);

            std::wprintf(L"Prod Name: %s\n", name_string);

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
