#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <cstddef>
#include <generator>
#include <memory>
#include <string_view>

#include "toml.hpp"
#include "system/controllerData.hpp"

namespace sys {

class Config {
   private:
    bool readed = false;
    toml::parse_result conf;
    std::size_t controllers_num = 0;
    Config() {}
    Config(Config const&) = delete;
    void operator=(Config const&) = delete;
    std::string constructDefaultPath();

   public:
    ~Config() = default;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    std::size_t getControllersNum() { return controllers_num; }
    std::shared_ptr<sys::System> parseConfig(std::string_view path = "");
    void initDummyFans(std::shared_ptr<sys::System> const& system) const;
    static void printConfig(std::shared_ptr<sys::System> const& system);
    bool isReaded() { return readed; }
    void setControllerNum(std::size_t cnum) { controllers_num = cnum; }

    void updateConf(std::shared_ptr<sys::System> const& system);
    void writeToFile(std::string_view path);
};

};  // namespace sys
#endif  // !__CONFIG_HPP__
