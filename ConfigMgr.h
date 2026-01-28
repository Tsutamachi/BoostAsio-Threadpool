#pragma once
//用于动态读取指定xml格式的config文件
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <fstream>
#include <iostream>
#include <map>

struct SectionInfo
{
    SectionInfo() {}
    ~SectionInfo() { _section_datas.clear(); }
    // 拷贝构造函数
    SectionInfo(const SectionInfo& src) { _section_datas = src._section_datas; }
    // 拷贝赋值函数
    SectionInfo& operator=(const SectionInfo& src)
    {
        if (&src == this) {
            return *this;
        }

        this->_section_datas = src._section_datas;
        return *this;
    }
    // ´需要将所有的参数读到map将键值对的行使
    std::map<std::string, std::string> _section_datas;
    // 重载括号的运算符
    std::string operator[](const std::string& key)
    {
        if (_section_datas.find(key) == _section_datas.end()) {
            return "";
        }
        // 找到了的话就返回值
        return _section_datas[key];
    }

    std::string GetValue(const std::string& key)
    {
        if (_section_datas.find(key) == _section_datas.end()) {
            return "";
        }
        return _section_datas[key];
    }
};

class ConfigMgr
{
public:
    ~ConfigMgr() { _config_map.clear(); }
    // 第一级别的[]
    SectionInfo operator[](const std::string& section)
    {
        if (_config_map.find(section) == _config_map.end()) {
            return SectionInfo();
        }
        return _config_map[section];
    }

    ConfigMgr& operator=(const ConfigMgr& src)
    {
        if (&src == this) {
            return *this;
        }
        this->_config_map = src._config_map;
    }

    ConfigMgr(const ConfigMgr& src) { this->_config_map = src._config_map; }

    static ConfigMgr& Inst()
    {
        // 局部静态变量生命周期与线程生命周期一样
        static ConfigMgr cfg_mgr;
        return cfg_mgr;
    }

    std::string GetValue(const std::string& section, const std::string& key);

private:
    // key为他的名字value是这个结构体
    ConfigMgr();
    std::map<std::string, SectionInfo> _config_map;
};
