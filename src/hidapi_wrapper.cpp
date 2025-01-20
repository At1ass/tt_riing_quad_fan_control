#include "hidapi_wrapper.hpp"
#include "hidapi.h"
#include "monitoring.hpp"
#include <cstddef>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <vector>

hid_device *dev;

void
hid_wrapper::device::init()
{
    this->dev = hid_open(THERMALTAKE_VID, pid, NULL);
}

void
hid_wrapper::device::send_init()
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
hid_wrapper::device::show_firmware_version()
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

    printf("Firmware version: %d.%d.%d\n", usb_buf[2], usb_buf[3], usb_buf[4]);
}

void
hid_wrapper::device::get_fan_data ( unsigned char       port,
                       unsigned char *     speed,
                       unsigned short *    rpm)
{
    unsigned char usb_buf[THERMALTAKE_QUAD_PACKET_SIZE];
    int ret;

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
    std::cout << "hid_write: " << ret << " | ";
    ret = hid_read_timeout(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE, THERMALTAKE_QUAD_INTERRUPT_TIMEOUT);
    std::cout << "hid_read: " << ret << "\n";
//    hid_read(dev, usb_buf, THERMALTAKE_QUAD_PACKET_SIZE);

    *speed = usb_buf[0x04];
    *rpm   = (usb_buf[0x06] << 8) + usb_buf[0x05];
}

void
hid_wrapper::device::send_fan ( unsigned char       port,
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
hid_wrapper::device::show_info() {
        wchar_t name_string[HID_MAX_STR];

        hid_get_manufacturer_string(dev, name_string, HID_MAX_STR);

        printf("Name: %ls\n", name_string);

        hid_get_product_string(dev, name_string, HID_MAX_STR);

        printf("Prod Name: %ls\n", name_string);

        for (size_t fan_index = 0; fan_index < THERMALTAKE_NUM_CHANNELS; fan_index++) {
            unsigned char speed;
            unsigned short rpm;

            get_fan_data(fan_index + 2, &speed, &rpm);

            printf("---- Fan %zu: speed - %hhu; rpm - %d\n",fan_index + 1, speed, rpm);
        }
}

void hid_wrapper::device::close_device()
{
    hid_close(dev);
}

void hid_wrapper::read_controllers() {
    struct hid_device_info *devs, *tmp;
    size_t i = 0;

    devs = hid_enumerate(THERMALTAKE_VID, 0);

    tmp = devs;
    while (tmp) {
        if (tmp->product_id >= 0x232b && tmp->product_id <= 0x232b + 3) {
            pids[i++] = tmp->product_id;
        }
        tmp = tmp->next;
    }

    fan_data.resize(pids.size(), std::vector<std::pair<unsigned char, unsigned short>>(5));

    hid_free_enumeration(devs);
}

void hid_wrapper::init_controllers() {
    for (auto &p : pids) {
        controllers.push_back(device(p));
    }
    for (auto &c : controllers) {
        c.init();
    }
}

void hid_wrapper::get_fan_data(int controller_id, int fan_id, unsigned char *speed, unsigned short *rpm) {
    controllers[controller_id].get_fan_data(fan_id, speed, rpm);
}

void hid_wrapper::update_fan_data() {
    unsigned char speed;
    unsigned short rpm;
    size_t s = controllers.size();
    failed = false;
    for (size_t c_idx = 0; c_idx < s; c_idx++) {
        for (size_t i = 0; i < 5; i++) {
            controllers[c_idx].get_fan_data(i + 1, &speed, &rpm);
            fan_data[c_idx][i].first = speed;
            fan_data[c_idx][i].second = rpm;
            if (rpm == 0 && i < 3) failed = true;
        }
    }
}

void hid_wrapper::send_to_controller(int controller_id, uint value) {
    for (size_t i = 1; i < 4; i++) {
        controllers[controller_id].send_fan(i, THERMALTAKE_FAN_MODE_FIXED, value);
    }
}

void hid_wrapper::sent_to_fan(int controller_id, int fan_index, uint value) {
        controllers[controller_id].send_fan(fan_index, THERMALTAKE_FAN_MODE_FIXED, value);
}

void hid_wrapper::send_to_all_controllers(uint value) {
    for (int i = 0; i < controllers.size(); i++) {
        send_to_controller(i, value);
    }
}

void hid_wrapper::close_controllers() {
    for (auto &c : controllers) {
        c.close_device();
    }
}


void
close_device()
{
    hid_close(dev);
}
void
init_hid()
{
    hid_init();
}

int
initialize_device(unsigned device_id)
{
    dev = hid_open(THERMALTAKE_VID, device_id, NULL);

    return dev != NULL;
}


void
close_hid()
{
    hid_exit();

}
