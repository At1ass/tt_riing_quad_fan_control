#ifndef __DUMMY_GPU__
#define __DUMMY_GPU__

#include "system/gpu.hpp"

namespace sys {

class DummyGPU : public GPU {
   public:
    DummyGPU();

    unsigned int getGPUTemp() override;
    std::string getGPUName() override;
    bool readGPUTemp(unsigned int& temp) override;
};

}  // namespace sys
#endif  // !__DUMMY_GPU__
