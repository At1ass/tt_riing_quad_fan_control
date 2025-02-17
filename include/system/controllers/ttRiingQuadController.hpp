#ifndef __TT_RIING_QUAD_CONTROLLER__
#define __TT_RIING_QUAD_CONTROLLER__

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include "hidapi.h"
#include "system/hidapi.hpp"
#include "system/hidapi_wrapper.hpp"

constexpr uint16_t const THERMALTAKE_VENDOR_ID = 0x264A;
constexpr uint16_t const TT_RIING_QUAD_START_PRODUCT_ID = 0x232B;
constexpr uint16_t const TT_RIING_QUAD_END_PRODUCT_ID = 0x232E;

constexpr std::size_t const TT_RIING_QUAD_NUM_CHANNELS = 5;
constexpr std::size_t const TT_RIING_QUAD_PACKET_SIZE = 193;
constexpr std::size_t const TT_RIING_QUAD_TIMEOUT = 250;
constexpr std::size_t const TT_RIING_QUAD_TIMEOUT_GET = 700;

constexpr float const COLOR_MULTIPLIER = 255.0F;

constexpr uint8_t const SHIFT = 8;

namespace sys {

enum ProtocolType : unsigned char {
    PROTOCOL_SET = 0x32,
    PROTOCOL_GET = 0x33,
    PROTOCOL_INIT = 0xFE,
    PROTOCOL_START_BYTE = 0x00
};

enum ProtocolTarget {
    PROTOCOT_FIRMWARE = 0x50,
    PROTOCOL_FAN = 0x51,
    PROTOCOL_LIGHT = 0x52,
};

enum ProtocolReturnValue { PROTOCOL_SUCCESS = 0xFC, PROTOCOL_FAIL = 0xFE };

enum ProtocolBytes { PROTOCOL_STATUS_BYTE = 0x02 };

enum ProtocolGetBytes {
    PROTOCOL_SPEED = 0x04,
    PROTOCOL_RPM_L = 0x05,
    PROTOCOL_RPM_H = 0x06
};

enum ProtocolModes : std::uint32_t { PROTOCOL_FAN_MODE_FIXED = 0x01U };

class TTRiingQuadController : public DeviceController {
   public:
    TTRiingQuadController(TTRiingQuadController const&) = delete;
    TTRiingQuadController(TTRiingQuadController&&) = delete;
    TTRiingQuadController& operator=(TTRiingQuadController const&) = delete;
    TTRiingQuadController& operator=(TTRiingQuadController&&) = delete;
    explicit TTRiingQuadController(std::unique_ptr<HidApi> hidapi)
        : hidapi_wrapper(std::move(hidapi)) {
        initControllers();
#ifdef ENABLE_INFO_LOGS
        showControllersInfo();
#endif  // ENABLE_INFO_LOGS
    }
    ~TTRiingQuadController() override {}

    std::pair<std::size_t, std::size_t> sentToFan(std::size_t controller_idx, std::size_t fan_idx,
                   uint value) override;

    void setRGB(std::size_t controller_idx, std::size_t fan_idx, std::array<float, 3>& colors) override;

    std::vector<std::vector<std::array<float, 3>>> makeColorBuffer() override;

    std::size_t controllersNum() { return devices.size(); }

   private:
    using device =
        std::unique_ptr<hid_device, std::function<void(hid_device*)>>;
    void initControllers();
    void showControllersInfo();
    void sendInit(device& dev);
    unsigned int convertChannel(float val);

    std::unique_ptr<HidApi> hidapi_wrapper;
    std::vector<device> devices;
};

}  // namespace sys
#endif  // !__TT_RIING_QUAD_CONTROLLER__
