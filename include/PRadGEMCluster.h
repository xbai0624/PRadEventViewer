#ifndef PRAD_GEM_CLUSTER_H
#define PRAD_GEM_CLUSTER_H

#include <string>
#include <unordered_map>
#include "PRadGEMPlane.h"
#include "PRadEventStruct.h"
#include "ConfigObject.h"

class PRadGEMDetector;

class PRadGEMCluster : public ConfigObject
{
public:
    PRadGEMCluster(const std::string &c_path = "");
    virtual ~PRadGEMCluster();

    // functions that to be overloaded
    void Configure(const std::string &path = "");

    virtual void Reconstruct(PRadGEMDetector *det);
    virtual void Reconstruct(PRadGEMPlane *plane);
    virtual void FormClusters(PRadGEMDetector *det);

protected:
    void clusterHits(std::vector<StripHit> &h, std::list<StripCluster> &c);
    void splitCluster(std::list<StripCluster> &c);
    void filterCluster(std::list<StripCluster> &c);
    void reconstructCluster(std::list<StripCluster> &c, PRadGEMPlane *p);
    bool filterCluster_sub(const StripCluster &c);
    bool splitCluster_sub(StripCluster &c, StripCluster &c1);
    void reconstructCluster_sub(StripCluster &c, PRadGEMPlane *p);

protected:
    // parameters
    unsigned int min_cluster_hits;
    unsigned int max_cluster_hits;
    float split_cluster_diff;
};

#endif
