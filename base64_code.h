#pragma once
#include <string>
#include <vector>

std::string base64_encode(const char *data, size_t length);
std::vector<char> base64_decode(const std::string &encoded_data);
