#ifndef __GPU_CONTROLLER__
#define __GPU_CONTROLLER__

#include "system/gpu.hpp"
#include <memory>
#include <string>

namespace sys {
    class IGPUController {
        public:
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
}

#endif // !__GPU_CONTROLLER__
