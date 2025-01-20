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

    /*std::vector<std::vector<std::map<float, float>>> fans;*/
    auto saved = config["saved"].as_array();
    if (saved != nullptr) {
        readed = true;
        for (auto &&d : *saved) {
            std::vector<std::map<float, float>> dd;
            for (auto &&c : *d.as_array()) {
                std::map<float, float> fd;
                for (auto &&f : *c.as_array()) {
                    std::cout << "[";
                    std::array<float, 2> tmp;
                    int i = 0;
                    for (auto &&q : *f.as_array()) {
                        std::cout << q.as_floating_point()->get() << " ";
                        tmp[i++] = q.as_floating_point()->get();
                    }
                    std::cout << "] ";
                    /*float x = f.as_array()[0].as_floating_point()->get();*/
                    /*float y = f.as_array()[1].as_floating_point()->get();*/
                    fd[tmp[0]] = tmp[1];
                }
                std::cout << "\n";
                dd.insert(dd.cend(), fd);
            }
            fans.insert(fans.cend(), dd);
        }


        std::cout << "-----READED------\n";
        for (auto &&i : fans) {
            for (auto &&j : i) {
                for (auto &&[t, s] : j) {
                    std::cout << "[" << t << ", " << s << "] ";
                }
                std::cout << "\n";
            }
        }
        return;
    }



    /*for (auto &&d : *saved) {*/
    /*    std::vector<std::map<float, float>> fanses;*/
    /*    std::map<float, float> fan;*/
    /*    for (auto &&f : *d.as_array()) {*/
    /*        toml::array *a = f.as_array();*/
    /*        float x = a[0].as_floating_point()->get();*/
    /*        float t = a[1].as_floating_point()->get();*/
    /*        fan[x] = t;*/
    /*    }*/
    /*}*/
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
}

std::map<std::string, std::vector<int>>
config::get_profiles() {
    return profiles;
}

void config::insert(std::vector<std::vector<std::map<float, float>>> fans) {
    toml::array saved;
    for (auto &&d : fans) {
        toml::array controller;
        for (auto &&f : d) {
            toml::array fan;
            for (auto &&[t, s] : f) {
                toml::array stat{t, s};
                fan.insert(fan.cend(), stat);
            }
            controller.insert(controller.cend(), fan);
        }
        saved.insert(saved.cend(), controller);
    }

    config.insert_or_assign("saved", saved);

    std::cout << toml::toml_formatter(config) << std::endl;

    std::fstream out;
    out.open("/home/at1ass/.config/config.toml", std::ios::out | std::ios::trunc);
    out  <<  toml::toml_formatter(config);
    out.close();

}
