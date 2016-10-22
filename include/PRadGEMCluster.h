#ifndef PRAD_GEM_CLUSTER_H
#define PRAD_GEM_CLUSTER_H

#include <string>
#include <unordered_map>
#include "PRadGEMPlane.h"
#include "ConfigParser.h"
#include "PRadEventStruct.h"

class PRadGEMDetector;

class PRadGEMCluster
{
public:
    PRadGEMCluster(const std::string &c_path = "");
    virtual ~PRadGEMCluster();

    std::string GetConfigPath() {return config_path;};
    ConfigValue GetConfigValue(const std::string &var_name);
    void SetConfigValue(const std::string &var_name, const ConfigValue &c_value);

    // functions that to be overloaded
    virtual void Configure(const std::string &path = "");
    virtual void Reconstruct(PRadGEMDetector *plane);
    virtual void Reconstruct(PRadGEMPlane *plane);
    virtual int FormClusters(GEMHit *hits, int MaxHits, PRadGEMPlane *x_plane, PRadGEMPlane *y_plane);

protected:
    void readConfigFile(const std::string &path);
    ConfigValue getConfigValue(const std::string &var_name,
                               const std::string &def_value,
                               bool verbose = true);
    void clusterHits(std::vector<GEMPlaneHit> &h, std::list<GEMPlaneCluster> &c);
    void splitCluster(std::list<GEMPlaneCluster> &c);
    void filterCluster(std::list<GEMPlaneCluster> &c);
    void reconstructCluster(std::list<GEMPlaneCluster> &c, PRadGEMPlane *p);
    bool filterCluster_sub(const GEMPlaneCluster &c);
    bool splitCluster_sub(GEMPlaneCluster &c, GEMPlaneCluster &c1);
    void reconstructCluster_sub(GEMPlaneCluster &c, PRadGEMPlane *p);

protected:
    std::string config_path;
    // configuration map
    std::unordered_map<std::string, ConfigValue> config_map;

    // parameters
    unsigned int min_cluster_hits;
    unsigned int max_cluster_hits;
    double split_cluster_diff;
};

#endif
