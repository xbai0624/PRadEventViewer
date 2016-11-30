#ifndef PRAD_ISLAND_CLUSTER_H
#define PRAD_ISLAND_CLUSTER_H

#include <vector>
#include "PRadHyCalCluster.h"

//#define PRIMEX_METHOD

class PRadIslandCluster : public PRadHyCalCluster
{
public:
    PRadIslandCluster(const std::string &path = "");
    virtual ~PRadIslandCluster();
    PRadHyCalCluster *Clone();

    void Configure(const std::string &path);
    void FormCluster(std::vector<ModuleHit> &hits,
                     std::vector<ModuleCluster> &clusters) const;

protected:
    void groupHits(std::vector<ModuleHit> &hits,
                   std::vector<ModuleCluster> &clusters) const;
    bool fillClusters(ModuleHit &hit, std::vector<ModuleCluster> &clusters) const;
    void splitClusters(std::vector<ModuleCluster> &clusters) const;
    void splitCluster(ModuleCluster &c1, ModuleCluster &c2) const;
#ifdef PRIMEX_METHOD
    bool checkAdjacent(const ModuleCluster &c1, const ModuleCluster &c2) const;
#endif

protected:
    // parameters for reconstruction
    float adj_dist;
    std::vector<float> min_module_energy;
};

#endif
