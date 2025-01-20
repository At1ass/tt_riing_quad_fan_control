#ifndef __HIDAPI_WRAPPER_HPP__
#define __HIDAPI_WRAPPER_HPP__

#include <hidapi.h>
#include <memory>
#include <sys/types.h>
#include <vector>
#include <array>

#define THERMALTAKE_VID     0x264a
#define HID_MAX_STR         256
#define THERMALTAKE_QUAD_PACKET_SIZE 193
#define THERMALTAKE_QUAD_INTERRUPT_TIMEOUT 250
#define THERMALTAKE_NUM_CHANNELS 5

enum
{
    THERMALTAKE_FAN_MODE_FIXED      = 0x01,
    THERMALTAKE_FAN_MODE_PWM        = 0x02
};

class hid_wrapper {
    class device {
        public:
            device(unsigned short pid) {
                this->pid = pid;
            }
            device(device& d) = delete;
            device(device&& d) noexcept :
                pid(std::move(d.pid)),
                dev(std::move(d.dev))
            {

            }
            void init();
            void send_init();
            void show_firmware_version();

            void get_fan_data ( unsigned char       port,
                                unsigned char *     speed,
                                unsigned short *    rpm);
            void send_fan ( unsigned char port,
                            unsigned char mode,
                            unsigned char speed);
            void show_info ();
            void close_device();

        private:
            unsigned short pid;
            hid_device* dev;
    };

private:
    std::vector<std::vector<std::pair<unsigned char, unsigned short>>> fan_data;
public:
    hid_wrapper() {
        hid_init();
        read_controllers();
        init_controllers();
    }
    size_t controllers_num() {
        return controllers.size();
    }
    void send_to_controller(int controller_id, uint value);
    void sent_to_fan(int controller_id, int fan_index, uint value);
    void get_fan_data(int controller_id, int fan_id, unsigned char *speed, unsigned short *rpm);
    void update_fan_data();
    const std::vector<std::vector<std::pair<unsigned char, unsigned short>>>& get_all_fan_data() {
        return fan_data;
    }
    bool failed = false;
    void send_to_all_controllers(uint value);
    ~hid_wrapper(){
        close_controllers();
        hid_exit();
    }
private:
    void read_controllers();
    void init_controllers();
    void close_controllers();

    std::array<int, 4> pids{0};
    std::vector<device> controllers;
};

void init_hid();

int initialize_device(unsigned device_id);

void send_init();

void show_firmware_version();

void get_fan_data (unsigned char port, unsigned char *speed, unsigned short *rpm);

void send_fan (unsigned char port, unsigned char mode, unsigned char speed);

void show_info ();

void close_device();

void close_hid();
#endif //__HIDAPI_WRAPPER_HPP__
