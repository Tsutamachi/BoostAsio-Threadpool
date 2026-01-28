#pragma once

//通过Base64将二进制数据转为字符串数据，用于JsonCpp格式数据传输
#include <string>
#include <vector>

std::string base64_encode(const char *data, size_t length);
std::vector<char> base64_decode(const std::string &encoded_data);
