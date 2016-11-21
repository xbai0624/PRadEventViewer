#ifndef PRAD_ISLAND_CLUSTER_H
#define PRAD_ISLAND_CLUSTER_H

#include <vector>
#include <list>
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
    void groupSectorHits(const std::vector<PRadHyCalModule*> &sector);
    bool fillCluster(PRadHyCalModule *m);
    bool checkContiguous(const PRadHyCalModule* c1, const PRadHyCalModule* c2) const;
    bool checkContiguous(const std::list<PRadHyCalModule*> &c1,
                         const std::list<PRadHyCalModule*> &c2) const;
    void splitSectorClusters();

protected:
    // parameters for reconstruction
    float min_module_energy;

    std::vector<std::vector<PRadHyCalModule*>> sector_hits;
    std::list<std::list<PRadHyCalModule*>> clusters;
};

#endif
