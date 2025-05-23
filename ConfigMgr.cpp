#include "ConfigMgr.h"
ConfigMgr::ConfigMgr()
{
    // 使用boost来管理文件读取,读取到当前文件路径
    boost::filesystem::path current_path = boost::filesystem::current_path();
    //进行拼接
    boost::filesystem::path config_path = current_path / "config.ini";
    std::cout << "Config path: " << config_path << std::endl;

    boost::property_tree::ptree pt;
    // 第一次参数是路径,的二个参数是结构
    boost::property_tree::read_ini(config_path.string(), pt);

    // 遍历先读取的是一一个section就是一次循环
    for (const auto& section_pair : pt) {
        // 第一个参数是section的名字
        const std::string& section_name = section_pair.first;
        // 第二个参数也是sectiontree这个类型可能有多个这样的结构
        const boost::property_tree::ptree& section_tree = section_pair.second;

        std::map<std::string, std::string> section_config;
        for (const auto& key_value_pair : section_tree) {
            const std::string& key = key_value_pair.first;
            const std::string& value = key_value_pair.second.get_value<std::string>();
            section_config[key] = value;
        }
        SectionInfo sectionInfo;
        sectionInfo._section_datas = section_config;
        _config_map[section_name] = sectionInfo;
    }

    for (const auto& section_entry : _config_map) {
        const std::string& section_name = section_entry.first;
        SectionInfo section_config = section_entry.second;
        std::cout << "[" << section_name << "]" << std::endl;
        for (const auto& key_value_pair : section_config._section_datas) {
            std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
        }
    }
}

std::string ConfigMgr::GetValue(const std::string& section, const std::string& key)
{
    if (_config_map.find(section) == _config_map.end()) {
        return "";
    }

    return _config_map[section].GetValue(key);
}
