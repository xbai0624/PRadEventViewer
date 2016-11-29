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
    void FormCluster(std::vector<ModuleHit> &hits,
                     std::vector<ModuleCluster> &clusters) const;

protected:
    void findCenters(std::vector<ModuleHit> &h, std::vector<ModuleCluster> &c) const;
    void fillClusters(std::vector<ModuleHit> &h, std::vector<ModuleCluster> &c) const;
    bool checkBelongs(const ModuleHit &center, const ModuleHit &hit, float factor) const;

protected:
    // parameters for reconstruction
    unsigned int square_size;
};

#endif
