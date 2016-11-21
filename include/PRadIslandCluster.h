#ifndef PRAD_ISLAND_CLUSTER_H
#define PRAD_ISLAND_CLUSTER_H

#include "PRadHyCalCluster.h"

class PRadIslandCluster : public PRadHyCalCluster
{
public:
    PRadIslandCluster(const std::string &path = "");
    virtual ~PRadIslandCluster();
    PRadHyCalCluster *Clone();

    void Configure(const std::string &path);
    void Reconstruct(PRadHyCalDetector *det);

protected:
    void fillSectorHits(PRadHyCalDetector *det);
    void groupSectorHits(const std::vector<ModuleHit> &sector);

protected:
    // parameters for reconstruction
    float min_module_energy;

    std::vector<std::vector<ModuleHit>> sector_hits;
};

#endif
