//============================================================================//
// Basic PRad Cluster Reconstruction Class For GEM                            //
// GEM Planes send hits infromation and container for the GEM Clusters to this//
// class reconstruct the clusters and send it back to GEM Planes.             //
// Thus the clustering algorithm can be adjusted in this class.               //
//                                                                            //
// Xinzhan Bai & Kondo Gnanvo, first version coding of the algorithm          //
// Chao Peng, adapted to PRad analysis software package                       //
// 10/21/2016                                                                 //
//============================================================================//

#include <iostream>
#include <iomanip>
#include <algorithm>
#include "ConfigObject.h"
#include "PRadGEMDetector.h"

// constructor
ConfigObject::ConfigObject(const std::string &ignore)
: ignore_chars(ignore)
{
    // place holder
}

// destructor
ConfigObject::~ConfigObject()
{
    // place holder
}

// configure the cluster method
void ConfigObject::Configure(const std::string & /*path*/)
{
    // to be overloaded
}

ConfigValue ConfigObject::GetConfigValue(const std::string &var_name)
{
    // convert to lower case and remove uninterested characters
    std::string key = ConfigParser::str_lower(ConfigParser::str_remove(var_name, ignore_chars));

    auto it = config_map.find(key);
    if(it == config_map.end())
        return ConfigValue("0");
    else
        return it->second;
}

void ConfigObject::SetConfigValue(const std::string &var_name, const ConfigValue &c_value)
{
    // convert to lower case and remove uninterested characters
    std::string key = ConfigParser::str_lower(ConfigParser::str_remove(var_name, ignore_chars));

    config_map[key] = c_value;
}

// read configuration file and build the configuration map
void ConfigObject::readConfigFile(const std::string &path)
{
    ConfigParser c_parser(":,=|"); // self-defined splitters

    if(!c_parser.OpenFile(path)) {
        std::cerr << "PRad HyCal Cluster Error: Cannot open file "
                  << "\"" << path << "\""
                  << std::endl;
        return;
    }

    // save the path
    config_path = path;

    // clear the map
    config_map.clear();

    while(c_parser.ParseLine())
    {
        if (c_parser.NbofElements() != 2)
            continue;

        std::string var_name, key;
        ConfigValue var_value;
        c_parser >> var_name >> var_value;

        // convert to lower case and remove uninterested characters
        key = ConfigParser::str_lower(ConfigParser::str_remove(var_name, ignore_chars));

        config_map[key] = var_value;
    }
}

// get configuration value from the map
ConfigValue ConfigObject::getConfigValue(const std::string &name,
                                         const std::string &def_value,
                                         bool verbose)
{
    std::string key = ConfigParser::str_lower(ConfigParser::str_remove(name, ignore_chars));

    auto it = config_map.find(key);
    if(it == config_map.end())
    {
        if(verbose) {
            std::cout << name << " (key: " << key << ")"
                      << " not defined in configuration file, set to default value "
                      << def_value
                      << std::endl;
        }
        config_map[key] = ConfigValue(def_value);
        return ConfigValue(def_value);
    }

    return it->second;
}
