#ifndef __GPU_HPP__
#define __GPU_HPP__

#include <string>

namespace sys {

class GPU {
   public:
    GPU(const GPU&) = default;
    GPU(GPU&&) = delete;
    GPU& operator=(const GPU&) = default;
    GPU& operator=(GPU&&) = delete;
    virtual ~GPU() = default;

    virtual unsigned int getGPUTemp() = 0;
    virtual std::string getGPUName() = 0;
    virtual bool readGPUTemp(unsigned int& temp) = 0;

   protected:
    GPU() = default;
    std::string gpu_name;     // NOLINT
    unsigned int gpu_temp{};  // NOLINT
};

}  // namespace sys
#endif  // __GPU_HPP__
