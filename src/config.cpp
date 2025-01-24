#include "config.hpp"
#include "toml.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void config::parse_config(std::string_view path) {

    if (profiles == nullptr
        || fans == nullptr) {
        profiles = std::make_shared<std::map<std::string, std::vector<int>>>();
        fans = std::make_shared<std::vector<std::vector<std::pair<int, std::map<float, float>>>>>();
    }

    conf = toml::parse_file(path);
    auto presets = conf["presets"].as_array();

    for (auto &&t : *presets) {
        toml::table* tbl = t.as_table();
        std::string name;
        std::vector<int> vals;

        tbl->for_each([&name, &vals](auto&& val) mutable
                      {
                            if (toml::is_string<decltype(val)>) {
                                name = val.as_string()->get();
                                return true;
                            }
                            else if (toml::is_array<decltype(val)>) {
                                val.as_array()->for_each([&vals](auto&&el) mutable
                                                        {
                                                            if (toml::is_integer<decltype(el)>) {
                                                                int64_t t = el.as_integer()->get();
                                                                vals.push_back(static_cast<int>(t));
                                                            }
                                                        });
                                return true;
                            }
                            else {
                                return false;
                            }
                      });

        profiles->emplace(name, vals);
    }

    auto saved = conf["saved"].as_array();
    if (fans->size() != 0) {
        fans->clear();
    }
    if (saved != nullptr) {
        readed = true;
        for (auto &&d : *saved) {
            std::vector<std::pair<int, std::map<float, float>>> dd;
            for (auto &&c : *d.as_array()) {
                std::map<float, float> fd;
                int mon;
                c.as_array()->for_each([=, this, &fd, &mon](auto& f) {
                                       if (toml::is_integer<decltype(f)>) {
                                            mon = f.as_integer()->get();
                                       }
                                       if (toml::is_array<decltype(f)>) {
                                            std::array<float, 2> tmp;
                                            int i = 0;
                                                for (auto &&q : *f.as_array()) {
                                                     tmp[i++] = q.as_floating_point()->get();
                                                }
                                            fd[tmp[0]] = tmp[1];
                                       }
                });
                dd.insert(dd.cend(), std::pair(mon, fd));
            }
            fans->insert(fans->cend(), dd);
        }
        return;
    }
}

void config::init_dummy_fans(int cnum) {
    fans->resize(cnum, std::vector<std::pair<int, std::map<float, float>>>(5));
    for (auto &&i : *fans) {
        for (auto &&j : i) {
            for (size_t idx = 0; idx <= 100; idx+=5) {
                j.first = 0;
                j.second[idx] = 50;
            }
        }
    }
}

void config::print_config() {
    std::cout << "Parsed config\n";
    for (auto &k : *profiles) {
        std::cout << k.first << std::endl;
        for (auto &e : k.second) {
            std::cout << e << " ";
        }
        std::cout << std::endl;
    }

    int i = 0;
    int j = 0;
    for (auto &&d : *fans) {
        std::cout << "Controller " << i + 1 << ":\n";
        j = 0;
        for (auto &&f : d) {
            std::cout << "Fan " << j + 1 << "(Monitoring " << (f.first ? "GPU" : "CPU") <<"): [";
            for (auto &&[t, s] : f.second) {
                std::cout << "[" << t <<", " << s << "] ";
            }
            j++;
            std::cout << "]\n";
        }
        i++;
    }
}

std::generator<const std::pair<float, float>&> config::get_next_fan_data(){
    for(auto &&d : *fans) {
        for (auto &&f : d) {
            for (auto &&data : f.second) {
                co_yield data;
            }
        }
    }
}

std::generator<const std::pair<std::pair<int, int>, std::pair<float, float>>&> config::get_next_fan_data_by_temp(float cpu_temp, float gpu_temp){
    int i = 0, j = 0;
    for(auto &&d : *fans) {
        for (auto &&f : d) {
            auto data = f.second.find(f.first ? gpu_temp : cpu_temp);
            co_yield {{i, j}, *data};
            j++;
        }
        i++;
    }
}

void config::update_conf() {
    toml::array saved;
    int i = 0;
    for (auto &&d : *fans) {
        toml::array controller;
        for (auto &&f : d) {
            toml::array fan;
            fan.insert(fan.cend(), f.first);
            for (auto &&[t, s] : f.second) {
                toml::array stat{t, s};
                fan.insert(fan.cend(), stat);
            }
            controller.insert(controller.cend(), fan);
        }
        saved.insert(saved.cend(), controller);
    }

    conf.insert_or_assign("saved", saved);
}

void config::write_to_file(std::string_view path) {
    std::fstream out;
    update_conf();
    print_config();
    out.open(path.data(), std::ios::out | std::ios::trunc);
    out  <<  toml::toml_formatter(conf);
    out.close();
}
