#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__
#include "toml.hpp"
#include <map>
#include <string_view>
#include <vector>

class config {
private:
    std::map<std::string, std::vector<int>> profiles;
    std::vector<std::vector<std::map<float, float>>> fans;
    bool readed = false;
    toml::parse_result config;

public:
    void parse_config(std::string_view path);
    void print_config();
    decltype(profiles) get_profiles();
    bool is_readed() {
        return readed;
    }
    auto& get_fans_settings(){
        return fans;
    }
    void insert(std::vector<std::vector<std::map<float, float>>> fans);
};
#endif // !__CONFIG_HPP__
