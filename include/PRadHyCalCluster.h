#ifndef PRAD_HYCAL_CLUSTER_H
#define PRAD_HYCAL_CLUSTER_H

#include <string>
#include <iostream>
#include "ConfigObject.h"
#include "PRadEventStruct.h"
#include "PRadHyCalDetector.h"

// we use 3x3 adjacent hits to reconstruct position
// here gives a larger volume to save the information
#define POS_RECON_HITS 15

class PRadHyCalCluster : public ConfigObject
{
public:
    virtual ~PRadHyCalCluster();
    virtual PRadHyCalCluster *Clone();
    virtual void Configure(const std::string &path);
    virtual void FormCluster(std::vector<ModuleHit> &hits,
                             std::vector<ModuleCluster> &clusters) const;
    virtual bool CheckCluster(const ModuleCluster &hit) const;

    HyCalHit Reconstruct(const ModuleCluster &cluster) const;
    float GetWeight(const float &E, const float &E0) const;
    float GetShowerDepth(int module_type, const float &E) const;
    void LeakCorr(ModuleCluster &cluster, std::vector<ModuleHit> &dead) const;

protected:
    PRadHyCalCluster();

    bool depth_corr;
    bool leak_corr;
    float log_weight_thres;
    float min_cluster_energy;
    float min_center_energy;
    float least_leak;
    unsigned int min_cluster_size;
    unsigned int leak_iters;
};

#endif
