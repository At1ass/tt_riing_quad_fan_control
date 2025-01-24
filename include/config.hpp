#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__
#include "toml.hpp"
#include <map>
#include <memory>
#include <string_view>
#include <vector>
#include <generator>

class config {
private:
    std::shared_ptr<std::map<std::string, std::vector<int>>> profiles = nullptr;
    std::shared_ptr<std::vector<std::vector<std::pair<int, std::map<float, float>>>>> fans = nullptr;
    bool readed = false;
    toml::parse_result conf;

public:
    void parse_config(std::string_view path);
    void init_dummy_fans(int cnum);
    void print_config();
    bool is_readed() {
        return readed;
    }

    int* get_fan_cpu_or_gpu(int i, int j) {
        return &(*fans)[i][j].first;
    }
    std::map<float, float>* get_fan_data(int i, int j) {
        return &(*fans)[i][j].second;
    }


    std::generator<const std::pair<float, float>&> get_next_fan_data();
    std::generator<const std::pair<std::pair<int, int>, std::pair<float, float>>&> get_next_fan_data_by_temp(float cpu_temp, float gpu_temp);

    decltype(profiles) get_profiles() {
        return profiles;
    }

    decltype(fans) get_fans_settings(){
        return fans;
    }

    void update_conf();
    void write_to_file(std::string_view path);
};
#endif // !__CONFIG_HPP__
