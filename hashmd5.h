#pragma once
#include <string>

std::string CalculateFileHash(const std::string &filepath);
std::string CalculateSmallFileHash(const std::string &filepath);
std::string CalculateLargeFileHash(const std::string &filepath);
// bool VerifyFileHash(const std::string &filepath, const std::stringstream clienthash);
bool VerifyFileHash(const std::string &filepath, const std::string clienthash);

// class HashMD5
// {
// public:

// };
