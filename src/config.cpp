#include "config.hpp"
#include "toml.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

void config::parse_config(std::string_view path) {
    config = toml::parse_file(path);
    auto presets = config["presets"].as_array();

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

        profiles.emplace(std::move(name), std::move(vals));
    }

    auto saved = config["saved"].as_array();
    if (saved != nullptr) {
        readed = true;
        for (auto &&d : *saved) {
            std::vector<std::map<float, float>> dd;
            for (auto &&c : *d.as_array()) {
                std::map<float, float> fd;
                c.as_array()->for_each([=, this, &fd](auto& f) {
                                       if (toml::is_integer<decltype(f)>) {
                                            is_cpu_or_gpu.push_back(f.as_integer()->get());
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
                dd.insert(dd.cend(), fd);
            }
            fans.insert(fans.cend(), dd);
        }
        return;
    }
}

void config::print_config() {
    std::cout << "Parsed config\n";
    for (auto &k : profiles) {
        std::cout << k.first << std::endl;
        for (auto &e : k.second) {
            std::cout << e << " ";
        }
        std::cout << std::endl;
    }

    int i = 0;
    int j = 0;
    for (auto &&d : fans) {
        std::cout << "Controller " << i + 1 << ":\n";
        for (auto &&f : d) {
            std::cout << "Fan " << j + 1 << "(Monitoring " << (is_cpu_or_gpu[i * 4 + j] ? "GPU" : "CPU") <<"): [";
            for (auto &&[t, s] : f) {
                std::cout << "[" << t <<", " << s << "] ";
            }
            std::cout << "]\n";
        }
    }
}

std::map<std::string, std::vector<int>>
config::get_profiles() {
    return profiles;
}

void config::insert(decltype(fans) fans, decltype(is_cpu_or_gpu) cpu_or_gpu) {
    toml::array saved;
    int i = 0;
    for (auto &&d : fans) {
        toml::array controller;
        for (auto &&f : d) {
            toml::array fan;
            fan.insert(fan.cend(), cpu_or_gpu[i++]);
            for (auto &&[t, s] : f) {
                toml::array stat{t, s};
                fan.insert(fan.cend(), stat);
            }
            controller.insert(controller.cend(), fan);
        }
        saved.insert(saved.cend(), controller);
    }

    config.insert_or_assign("saved", saved);

    std::fstream out;
    out.open("/home/at1ass/.config/config.toml", std::ios::out | std::ios::trunc);
    out  <<  toml::toml_formatter(config);
    out.close();

}
