#ifndef __STRING_UTILS_HPP__
#define __STRING_UTILS_HPP__

#include <string>

bool findInput(std::string const& path, char const* input_prefix,
                      std::string& input, std::string const& name);

bool findFallbackInput(std::string const& path, char const* input_prefix,
                              std::string& input);
#endif  //!__STRING_UTILS_HPP__
