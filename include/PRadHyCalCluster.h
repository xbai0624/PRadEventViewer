#ifndef PRAD_HYCAL_CLUSTER_H
#define PRAD_HYCAL_CLUSTER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "PRadEventStruct.h"
#include "PRadDataHandler.h"
#include "PRadDAQUnit.h"
#include "PRadTDCGroup.h"
#include "ConfigParser.h"
#include "PRadDetectors.h"

#define MAX_HCLUSTERS 250 // Maximum storage

class PRadDataHandler;

class PRadHyCalCluster
{
public:
    PRadHyCalCluster(PRadDataHandler *h = nullptr);
    virtual ~PRadHyCalCluster();

    void SetHandler(PRadDataHandler *h);
    std::string GetConfigPath() {return config_path;};
    ConfigValue GetConfigValue(const std::string &var_name);
    void SetConfigValue(const std::string &var_name, const ConfigValue &c_value);

    // functions that to be overloaded
    virtual void Configure(const std::string &path = "");
    virtual void Clear();
    virtual void Reconstruct(EventData &event);
    virtual int GetNClusters() {return fNHyCalClusters;};
    virtual HyCalHit *GetCluster() {return fHyCalCluster;};

protected:
    void readConfigFile(const std::string &path);
    ConfigValue getConfigValue(const std::string &var_name,
                               const std::string &def_value,
                               bool verbose = true);

protected:
    PRadDataHandler *fHandler;
    std::string config_path;
    // configuration map
    std::unordered_map<std::string, ConfigValue> fConfigMap;
    // result array
    HyCalHit fHyCalCluster[MAX_HCLUSTERS];
    int fNHyCalClusters;
};

#endif
