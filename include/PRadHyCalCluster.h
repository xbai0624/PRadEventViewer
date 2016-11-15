#ifndef PRAD_HYCAL_CLUSTER_H
#define PRAD_HYCAL_CLUSTER_H

#include "ConfigObject.h"

class PRadHyCalCluster : public ConfigObject
{
public:
    PRadHyCalCluster();
    virtual ~PRadHyCalCluster();

    virtual void Reconstruct(PRadHyCalCluster *);
    virtual void Clear();
};

#endif
