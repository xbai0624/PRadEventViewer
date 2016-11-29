#ifndef PRAD_HYCAL_CLUSTER_H
#define PRAD_HYCAL_CLUSTER_H

#include <string>
#include <iostream>
#include "ConfigObject.h"
#include "PRadEventStruct.h"
#include "PRadHyCalDetector.h"

class PRadHyCalCluster : public ConfigObject
{
public:
    virtual ~PRadHyCalCluster();
    virtual PRadHyCalCluster *Clone();
    virtual void Configure(const std::string &path);
    virtual void FormCluster(std::vector<ModuleHit> &hits,
                             std::vector<ModuleCluster> &clusters) const;
    virtual bool CheckCluster(const ModuleCluster &hit) const;

    float GetWeight(const float &E, const float &E0) const;
    float GetShowerDepth(int module_type, const float &E) const;
    HyCalHit Reconstruct(const ModuleCluster &cluster) const;

protected:
    PRadHyCalCluster();

    bool depth_corr;
    float log_weight_thres;
    float min_cluster_energy;
    float min_center_energy;
    unsigned int min_cluster_size;
};

#endif
