#ifndef __GPU_HPP__
#define __GPU_HPP__

#include <string>

namespace sys {
    class GPU {
        public:
            virtual ~GPU() = default;

            virtual float getGPUTemp() = 0;
            virtual std::string getGPUName() = 0;
            virtual bool readGPUTemp(unsigned int& temp) = 0;

        protected:
            std::string gpu_name;
            float gpu_temp;
    };
}

#endif // __GPU_HPP__
