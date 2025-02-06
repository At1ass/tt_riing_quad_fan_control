#ifndef __AMD_HPP__
#define __AMD_HPP__

#include "system/gpu.hpp"
#include <fstream>


namespace sys {
    class AMD : public GPU {
        public:
            AMD(const std::string& card);
            ~AMD();
            float getGPUTemp() override;
            std::string getGPUName() override;
            bool readGPUTemp(unsigned int& temp) override;
        private:
            void findGPUTempFile(const std::string& card);
            bool calculateGPUName();
            std::ifstream gpu_temp_file;
    };
}

#endif // !__AMD_HPP__
