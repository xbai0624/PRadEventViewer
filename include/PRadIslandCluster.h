#ifndef PRAD_ISLAND_CLUSTER_H
#define PRAD_ISLAND_CLUSTER_H

#include <list>
#include <vector>
#include "PRadHyCalCluster.h"

// value to judge if two modules are connected at corner, quantized to module size
#define CORNER_ADJACENT 1.6
// value to judge if two modules are sharing a side line
#define SIDE_ADJACENT 1.3

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
#ifdef PRIMEX_METHOD
    void groupHits(std::vector<ModuleHit> &hits,
                   std::vector<std::vector<ModuleHit*>> &groups) const;
    bool fillClusters(ModuleHit &hit, std::vector<std::vector<ModuleHit*>> &groups) const;
    bool checkAdjacent(const std::vector<ModuleHit*> &g1, const std::vector<ModuleHit*> &g2) const;
    void splitCluster(std::vector<ModuleHit*> &group, std::vector<ModuleCluster> &clusters) const;
#else
    void groupHits(std::vector<ModuleHit> &hits,
                   std::vector<ModuleCluster> &clusters) const;
    bool fillClusters(ModuleHit &hit, std::vector<ModuleCluster> &clusters) const;
    bool splitHit(ModuleHit &hit,
                  std::vector<ModuleCluster> &clusters,
                  std::vector<unsigned int> &indices) const;
#endif

protected:
    // parameters for reconstruction
    float adj_dist;
    std::vector<float> min_module_energy;
};

#endif
