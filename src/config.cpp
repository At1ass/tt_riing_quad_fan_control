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
        || fans == nullptr
        || is_cpu_or_gpu == nullptr) {
        profiles = std::make_shared<std::map<std::string, std::vector<int>>>();
        fans = std::make_shared<std::vector<std::vector<std::map<float, float>>>>();
        is_cpu_or_gpu = std::make_shared<std::vector<int>>();
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
    if (fans->size() != 0 || is_cpu_or_gpu->size() != 0) {
        fans->clear();
        is_cpu_or_gpu->clear();
    }
    if (saved != nullptr) {
        readed = true;
        for (auto &&d : *saved) {
            std::vector<std::map<float, float>> dd;
            for (auto &&c : *d.as_array()) {
                std::map<float, float> fd;
                c.as_array()->for_each([=, this, &fd](auto& f) {
                                       if (toml::is_integer<decltype(f)>) {
                                            is_cpu_or_gpu->push_back(f.as_integer()->get());
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
            fans->insert(fans->cend(), dd);
        }
        return;
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
            std::cout << "Fan " << j + 1 << "(Monitoring " << ((*is_cpu_or_gpu)[i * 4 + j] ? "GPU" : "CPU") <<"): [";
            for (auto &&[t, s] : f) {
                std::cout << "[" << t <<", " << s << "] ";
            }
            j++;
            std::cout << "]\n";
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
            fan.insert(fan.cend(), (*is_cpu_or_gpu)[i++]);
            for (auto &&[t, s] : f) {
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
