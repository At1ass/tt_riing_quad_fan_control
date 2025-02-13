#ifndef __HIDAPI_WRAPPER_HPP__
#define __HIDAPI_WRAPPER_HPP__

#include <hidapi.h>
#include <sys/types.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#define THERMALTAKE_VID 0x264a
#define HID_MAX_STR 256
#define THERMALTAKE_QUAD_PACKET_SIZE 193
#define THERMALTAKE_QUAD_INTERRUPT_TIMEOUT 250
#define THERMALTAKE_NUM_CHANNELS 5

namespace sys {

enum { THERMALTAKE_FAN_MODE_FIXED = 0x01, THERMALTAKE_FAN_MODE_PWM = 0x02 };

class IHidWrapper {
   public:
    IHidWrapper(IHidWrapper const&) = default;
    IHidWrapper(IHidWrapper&&) = delete;
    IHidWrapper& operator=(IHidWrapper const&) = default;
    IHidWrapper& operator=(IHidWrapper&&) = delete;
    virtual ~IHidWrapper() = default;
    virtual void sentToFan(std::size_t controller_idx, std::size_t fan_idx,
                           uint value) = 0;

   protected:
    IHidWrapper() = default;
};

class HidWrapper : public IHidWrapper {
    class Device {
       public:
        Device& operator=(Device const&) = delete;
        Device& operator=(Device&&) = delete;
        explicit Device(uint16_t pid) : pid(pid) {}
        Device(Device& d) = delete;
        Device(Device&& d) noexcept
            : pid(std::move(d.pid)), dev(std::move(d.dev)) {}
        void init();
        void sendInit();
        void showFirmwareVersion();

        void getFanData(unsigned char port, unsigned char* speed,
                        uint16_t* rpm);
        void sendFan(unsigned char port, unsigned char mode,
                     unsigned char speed);
        void showInfo();
        void closeDevice();
        ~Device();

       private:
        template <typename... T>
        void sendRequest(unsigned char f_prot, unsigned char s_prot,
                         T... data) {
            std::array<unsigned char, THERMALTAKE_QUAD_PACKET_SIZE> usb_buf{
                0x00, f_prot, s_prot, data...};
            int ret = 0;

            ret = hid_write(dev, usb_buf.data(), THERMALTAKE_QUAD_PACKET_SIZE);
            if (ret == -1) {
                throw std::runtime_error(
                    constructError("Failed hid_write: ", hid_error(dev)));
            }
        }

        std::array<unsigned char, THERMALTAKE_QUAD_PACKET_SIZE> readResponse() {
            std::array<unsigned char, THERMALTAKE_QUAD_PACKET_SIZE> response{0};
            int ret = 0;

            ret = hid_read_timeout(dev, response.data(),
                                   THERMALTAKE_QUAD_PACKET_SIZE,
                                   THERMALTAKE_QUAD_INTERRUPT_TIMEOUT);
            if (ret == -1) {
                throw std::runtime_error(constructError(
                    "Failed hid_read_timeout: ", hid_error(dev)));
            }

            return response;
        }

        std::string constructError(std::string const& msg,
                                   wchar_t const* reason);
        uint16_t pid;
        hid_device* dev{};
    };

   private:
    std::vector<std::vector<std::pair<unsigned char, uint16_t>>> fan_data;

   public:
    HidWrapper() {
        int ret = hid_init();

        if (ret != 0) {
            throw std::runtime_error("Cannot iniztialize hidapi");
        }

        readControllers();
        initControllers();
#if ENABLE_INFO_LOGS
        showControllersInfo();
#endif
    }
    HidWrapper(HidWrapper const&) = default;
    HidWrapper(HidWrapper&&) = delete;
    HidWrapper& operator=(HidWrapper const&) = default;
    HidWrapper& operator=(HidWrapper&&) = delete;
    std::size_t controllersNum() { return controllers.size(); }
    void sendToController(int controller_id, uint value);
    void sentToFan(std::size_t controller_id, std::size_t fan_index,
                   uint value) override;
    void getFanData(int controller_id, int fan_id, unsigned char* speed,
                    uint16_t* rpm);
    void updateFanData();
    std::vector<std::vector<std::pair<unsigned char, uint16_t>>> const&
    getAllFanData() {
        return fan_data;
    }
    void sendToAllControllers(uint value);
    ~HidWrapper() override {
        closeControllers();
        hid_exit();
    }

   private:
    void readControllers();
    void initControllers();
    void closeControllers();
    void showControllersInfo();

    bool failed = false;
    std::array<int, 4> pids{0};
    std::vector<Device> controllers;
};

}  // namespace sys
#endif  //__HIDAPI_WRAPPER_HPP__
