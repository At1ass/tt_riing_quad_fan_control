#ifndef __CPU_CONTROLLER__
#define __CPU_CONTROLLER__

#include <fstream>

namespace sys {
class ICPUController {
   public:
    ICPUController(ICPUController const&) = default;
    ICPUController(ICPUController&&) = delete;
    ICPUController& operator=(ICPUController const&) = default;
    ICPUController& operator=(ICPUController&&) = delete;
    virtual ~ICPUController() = default;
    virtual bool readCpuTempFile(int& temp) = 0;
    virtual std::string getCPUName() = 0;

   protected:
    ICPUController() = default;
};

class CPUController : public ICPUController {
   public:
    CPUController();
    CPUController(CPUController const&) = delete;
    CPUController(CPUController&&) = delete;
    CPUController& operator=(CPUController const&) = delete;
    CPUController& operator=(CPUController&&) = delete;
    ~CPUController() override;
    bool readCpuTempFile(int& temp) override;
    std::string getCPUName() override;

   private:
    std::ifstream cpu_file;
    std::string cpu_name;

    bool getCPUFile();
    void cpuInfoCpuName();
};
}  // namespace sys

#endif  // __CPU_CONTROLLER__
