#ifndef __AMD_HPP__
#define __AMD_HPP__

#include <fstream>

#include "system/gpu.hpp"

namespace sys {

class AMD : public GPU {
   public:
    AMD(const AMD&) = delete;
    AMD(AMD&&) = delete;
    AMD& operator=(const AMD&) = delete;
    AMD& operator=(AMD&&) = delete;
    explicit AMD(std::string const& card);
    ~AMD() override;
    unsigned int getGPUTemp() override;
    std::string getGPUName() override;
    bool readGPUTemp(unsigned int& temp) override;

   private:
    void findGPUTempFile(std::string const& card);
    bool calculateGPUName();
    std::ifstream gpu_temp_file;
};

}  // namespace sys
#endif  // !__AMD_HPP__
