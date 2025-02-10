#ifndef __CPU_CONTROLLER__
#define __CPU_CONTROLLER__

#include <fstream>

namespace sys {
    class ICPUController {
        public:
            virtual ~ICPUController() = default;
            virtual bool readCpuTempFile(int &temp) = 0;
            virtual std::string getCPUName() = 0;
    };

    class CPUController : public ICPUController {
        public:
            CPUController();
            ~CPUController();
            bool readCpuTempFile(int &temp) override;
            std::string getCPUName() override;
        private:
            std::ifstream cpu_file;
            std::string cpu_name;

            bool getCPUFile();
            void cpuInfoCpuName();
    };
}

#endif // __CPU_CONTROLLER__
