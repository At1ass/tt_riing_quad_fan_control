#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__
#include "toml.hpp"
#include <map>
#include <memory>
#include <string_view>
#include <vector>

class config {
private:
    std::shared_ptr<std::map<std::string, std::vector<int>>> profiles = nullptr;
    std::shared_ptr<std::vector<std::vector<std::map<float, float>>>> fans = nullptr;
    std::shared_ptr<std::vector<int>> is_cpu_or_gpu = nullptr;
    bool readed = false;
    toml::parse_result conf;

public:
    void parse_config(std::string_view path);
    void print_config();
    bool is_readed() {
        return readed;
    }

    //std::map<std::string, std::vector<int>> get_profiles() {
    decltype(profiles) get_profiles() {
        return profiles;
    }

    //std::vector<std::vector<std::map<float, float>>>& get_fans_settings(){
    decltype(fans) get_fans_settings(){
        return fans;
    }
    //std::vector<int>& get_cpu_or_gpu() {
    decltype(is_cpu_or_gpu) get_cpu_or_gpu() {
        return is_cpu_or_gpu;
    }
    void update_conf();
    void write_to_file(std::string_view path);
};
#endif // !__CONFIG_HPP__
