#ifndef PRAD_SQUARE_CLUSTER_H
#define PRAD_SQUARE_CLUSTER_H

#include "PRadHyCalCluster.h"

class PRadSquareCluster : public PRadHyCalCluster
{
public:
    PRadSquareCluster(const std::string &path = "");
    virtual ~PRadSquareCluster();
    PRadHyCalCluster *Clone();

    void Configure(const std::string &path);
    std::vector<ModuleCluster> Reconstruct(std::list<ModuleHit> &hits) const;

protected:
    void findCenters(std::list<ModuleHit> &h, std::vector<ModuleCluster> &c) const;
    void fillClusters(std::list<ModuleHit> &h, std::vector<ModuleCluster> &c) const;


protected:
    // parameters for reconstruction
    unsigned int square_size;
};

#endif
