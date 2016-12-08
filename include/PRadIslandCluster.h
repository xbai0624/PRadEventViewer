#ifndef PRAD_ISLAND_CLUSTER_H
#define PRAD_ISLAND_CLUSTER_H

#include <list>
#include <vector>
#include "PRadHyCalCluster.h"

#define PRIMEX_METHOD

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
// primex method
#ifdef PRIMEX_METHOD
    void groupHits(std::vector<ModuleHit> &hits,
                   std::vector<std::vector<ModuleHit*>> &groups) const;
    bool fillClusters(ModuleHit &hit, std::vector<std::vector<ModuleHit*>> &groups) const;
    bool checkAdjacent(const std::vector<ModuleHit*> &g1, const std::vector<ModuleHit*> &g2) const;
    void splitCluster(const std::vector<ModuleHit*> &grp, std::vector<ModuleCluster> &c) const;
    std::vector<ModuleHit*> findMaximums(const std::vector<ModuleHit*> &g) const;
    void splitHits(const std::vector<ModuleHit*> &maximums,
                   const std::vector<ModuleHit*> &hits,
                   std::vector<ModuleCluster> &clusters) const;
    void evalFraction(const std::vector<ModuleHit*> &maximums,
                      const std::vector<ModuleHit*> &hits,
                      size_t iters) const;
// M. Levillain and W. Xiong method
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
    unsigned int split_iter;
    float adj_dist;
    float least_share;
    std::vector<float> min_module_energy;
};

#endif
