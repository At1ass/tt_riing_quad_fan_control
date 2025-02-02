#ifndef __HIDAPI_WRAPPER_HPP__
#define __HIDAPI_WRAPPER_HPP__

#include <hidapi.h>
#include <sys/types.h>
#include <vector>
#include <array>

#define THERMALTAKE_VID     0x264a
#define HID_MAX_STR         256
#define THERMALTAKE_QUAD_PACKET_SIZE 193
#define THERMALTAKE_QUAD_INTERRUPT_TIMEOUT 250
#define THERMALTAKE_NUM_CHANNELS 5

namespace sys {
    enum
    {
        THERMALTAKE_FAN_MODE_FIXED      = 0x01,
        THERMALTAKE_FAN_MODE_PWM        = 0x02
    };

    class HidWrapper {
        class Device {
            public:
                Device(unsigned short pid) {
                    this->pid = pid;
                }
                Device(Device& d) = delete;
                Device(Device&& d) noexcept :
                    pid(std::move(d.pid)),
                    dev(std::move(d.dev))
                    {

                    }
                void init();
                void sendInit();
                void showFirmwareVersion();

                void getFanData ( unsigned char       port,
                        unsigned char *     speed,
                        unsigned short *    rpm);
                void sendFan ( unsigned char port,
                        unsigned char mode,
                        unsigned char speed);
                void showInfo ();
                void closeDevice();

            private:
                unsigned short pid;
                hid_device* dev;
        };

        private:
        std::vector<std::vector<std::pair<unsigned char, unsigned short>>> fan_data;
        public:
        HidWrapper() {
            hid_init();
            readControllers();
            initControllers();
            showControllersInfo();
        }
        size_t controllersNum() { return controllers.size(); }
        void sendToController(int controller_id, uint value);
        void sentToFan(int controller_id, int fan_index, uint value);
        void getFanData(int controller_id, int fan_id, unsigned char *speed, unsigned short *rpm);
        void updateFanData();
        const std::vector<std::vector<std::pair<unsigned char, unsigned short>>>& getAllFanData() {
            return fan_data;
        }
        bool failed = false;
        void sendToAllControllers(uint value);
        ~HidWrapper(){
            closeControllers();
            hid_exit();
        }
        private:
        void readControllers();
        void initControllers();
        void closeControllers();
        void showControllersInfo();

        std::array<int, 4> pids{0};
        std::vector<Device> controllers;
    };
}
#endif //__HIDAPI_WRAPPER_HPP__
