#ifndef PRAD_ISLAND_CLUSTER_H
#define PRAD_ISLAND_CLUSTER_H

#include <vector>
#include "PRadHyCalCluster.h"

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
    void groupSectorHits(std::vector<ModuleHit> &hits,
                         std::vector<ModuleCluster> &clusters) const;
    bool fillClusters(ModuleHit &hit, std::vector<ModuleCluster> &clusters) const;
    bool checkContiguous(const ModuleHit &m1, const ModuleHit &m2) const;
    void splitSectorClusters();

protected:
    // parameters for reconstruction
    std::vector<float> min_module_energy;
};

#endif
