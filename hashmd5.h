#pragma once

class HashMD5
{
public:
    std::string CalculateFileHash(const std::string &filepath);
    std::string CalculateSmallFileHash(const std::string &filepath);
    std::string CalculateLargeFileHash(const std::string &filepath);
    bool VerifyFileHash(const std::string &filepath, const std::stringstream clienthash);
};
