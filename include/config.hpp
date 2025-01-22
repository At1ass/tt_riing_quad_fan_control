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
    std::vector<int> is_cpu_or_gpu;
    bool readed = false;
    toml::parse_result config;

public:
    void parse_config(std::string_view path);
    void print_config();
    decltype(profiles) get_profiles();
    bool is_readed() {
        return readed;
    }
    std::vector<std::vector<std::map<float, float>>>& get_fans_settings(){
        return fans;
    }
    std::vector<int>& get_cpu_or_gpu() {
        return is_cpu_or_gpu;
    }
    void insert(decltype(fans) fans, decltype(is_cpu_or_gpu) cpu_or_gpu);
    void write_to_file(std::string_view path);
};
#endif // !__CONFIG_HPP__
