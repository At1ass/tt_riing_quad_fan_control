#ifndef __GPU_CONTROLLER__
#define __GPU_CONTROLLER__

#include <memory>
#include <string>

#include "system/gpu.hpp"

namespace sys {

class IGPUController {
   public:
    IGPUController(IGPUController const&) = default;
    IGPUController(IGPUController&&) = delete;
    IGPUController& operator=(IGPUController const&) = default;
    IGPUController& operator=(IGPUController&&) = delete;
    IGPUController() = default;
    virtual ~IGPUController() = default;
    virtual float getGPUTemp() = 0;
    virtual std::string getGPUName() = 0;
    virtual bool readGPUTemp(unsigned int& temp) = 0;
};

class GPUController : public IGPUController {
   public:
    GPUController();
    float getGPUTemp() override;
    std::string getGPUName() override;
    bool readGPUTemp(unsigned int& temp) override;

   private:
    std::unique_ptr<GPU> gpu;
};

}  // namespace sys
#endif  // !__GPU_CONTROLLER__
