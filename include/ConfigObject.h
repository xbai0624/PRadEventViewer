#ifndef CONFIG_OBJECT_H
#define CONFIG_OBJECT_H

#include <string>
#include <unordered_map>
#include "ConfigParser.h"

class ConfigObject
{
public:
    ConfigObject(const std::string &ignore = " _\t");
    virtual ~ConfigObject();

    std::string GetConfigPath() {return config_path;};
    ConfigValue GetConfigValue(const std::string &var_name);
    void SetConfigValue(const std::string &var_name, const ConfigValue &c_value);

    // functions that to be overloaded
    virtual void Configure(const std::string &path = "");

protected:
    void readConfigFile(const std::string &path);
    ConfigValue getConfigValue(const std::string &var_name,
                               const std::string &def_value,
                               bool verbose = true);

protected:
    std::string ignore_chars;
    std::string config_path;
    std::unordered_map<std::string, ConfigValue> config_map;
};

#endif
