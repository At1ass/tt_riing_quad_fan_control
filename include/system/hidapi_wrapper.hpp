#ifndef __HIDAPI_WRAPPER_HPP__
#define __HIDAPI_WRAPPER_HPP__

#include <hidapi.h>
#include <sys/types.h>

#include <cstring>
#include <utility>
#include <vector>

namespace sys {

class DeviceController {
   public:
    DeviceController(DeviceController const&) = default;
    DeviceController(DeviceController&&) = delete;
    DeviceController& operator=(DeviceController const&) = default;
    DeviceController& operator=(DeviceController&&) = delete;
    virtual ~DeviceController() = default;
    virtual std::pair<std::size_t, std::size_t> sentToFan(std::size_t controller_idx, std::size_t fan_idx,
                           uint value) = 0;
    virtual void setRGB(std::size_t controller_idx, std::size_t fan_idx, std::array<float, 3>& colors) = 0;
    virtual std::vector<std::vector<std::array<float, 3>>> makeColorBuffer() = 0;

   protected:
    DeviceController() = default;
};

}  // namespace sys
#endif  //__HIDAPI_WRAPPER_HPP__
