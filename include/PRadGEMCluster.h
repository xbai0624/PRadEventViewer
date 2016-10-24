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
    virtual int FormClusters(PRadGEMDetector *det);

protected:
    void clusterHits(std::vector<GEMPlaneHit> &h, std::list<GEMPlaneCluster> &c);
    void splitCluster(std::list<GEMPlaneCluster> &c);
    void filterCluster(std::list<GEMPlaneCluster> &c);
    void reconstructCluster(std::list<GEMPlaneCluster> &c, PRadGEMPlane *p);
    bool filterCluster_sub(const GEMPlaneCluster &c);
    bool splitCluster_sub(GEMPlaneCluster &c, GEMPlaneCluster &c1);
    void reconstructCluster_sub(GEMPlaneCluster &c, PRadGEMPlane *p);

protected:
    // parameters
    unsigned int min_cluster_hits;
    unsigned int max_cluster_hits;
    double split_cluster_diff;
};

#endif
