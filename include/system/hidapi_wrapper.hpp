#ifndef __HIDAPI_WRAPPER_HPP__
#define __HIDAPI_WRAPPER_HPP__

#include <hidapi.h>
#include <sys/types.h>

#include <cstring>

namespace sys {

class DeviceController {
   public:
    DeviceController(DeviceController const&) = default;
    DeviceController(DeviceController&&) = delete;
    DeviceController& operator=(DeviceController const&) = default;
    DeviceController& operator=(DeviceController&&) = delete;
    virtual ~DeviceController() = default;
    virtual void sentToFan(std::size_t controller_idx, std::size_t fan_idx,
                           uint value) = 0;
    virtual void setRGB(std::size_t controller_idx, std::size_t fan_idx) = 0;

   protected:
    DeviceController() = default;
};

}  // namespace sys
#endif  //__HIDAPI_WRAPPER_HPP__
