#ifndef PRAD_SQUARE_CLUSTER_H
#define PRAD_SQUARE_CLUSTER_H

#include "PRadHyCalCluster.h"

class PRadSquareCluster : public PRadHyCalCluster
{
public:
    PRadSquareCluster(const std::string &path = "");
    virtual ~PRadSquareCluster();
    PRadHyCalCluster *Clone();

    void Configure(const std::string &path = "");
    void Reconstruct(PRadHyCalDetector *det);

protected:
    std::vector<PRadHyCalModule*> findCenters(const PRadHyCalDetector *det);
    HyCalHit formCluster(PRadHyCalModule *center, const PRadHyCalDetector *det);

protected:
    // parameters for reconstruction
    unsigned int square_size;
};

#endif
