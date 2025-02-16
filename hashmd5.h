#pragma once
#include <string>
#include <vector>

std::string CalculateFileHash(const std::string &filepath);
std::string CalculateSmallFileHash(const std::string &filepath);
std::string CalculateLargeFileHash(const std::string &filepath);
std::string CalculateBlockHash(const std::vector<char> &input);
bool VerifyFileHash(const std::string &filepath, const std::string clienthash);
