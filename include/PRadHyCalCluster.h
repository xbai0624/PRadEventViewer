#ifndef PRAD_HYCAL_CLUSTER_H
#define PRAD_HYCAL_CLUSTER_H

#include <string>
#include "ConfigObject.h"
#include "PRadEventStruct.h"
#include "PRadHyCalDetector.h"

class PRadHyCalCluster : public ConfigObject
{
public:
    struct ModuleHit
    {
        int id;                         // module id
        float x;                        // x in detector frame
        float y;                        // y in detector frame
        float energy;                   // participated energy, may be splitted

        ModuleHit() : id(0), energy(0) {};
        ModuleHit(int a, float e) : id(a), energy(e) {};
    };

    struct ModuleCluster
    {
        int cid;                        // center module id
        int det_id;                     // detector id
        unsigned int flag;              // cluster status flag
        std::vector<ModuleHit> hits;    // hits group
    };

public:
    virtual ~PRadHyCalCluster();
    virtual PRadHyCalCluster *Clone();
    virtual void Configure(const std::string &path);
    virtual void Reconstruct(PRadHyCalDetector *det);
    virtual float GetWeight(const float &E, const float &E0);
    virtual float GetShowerDepth(int module_type, const float &E);
    virtual void NonLinearCorr(PRadHyCalModule *center, float &E);
    virtual bool CheckCluster(const HyCalCluster &hit);

protected:
    PRadHyCalCluster();

    bool depth_corr;
    bool non_linear_corr;
    float log_weight_thres;
    float min_cluster_energy;
    float min_center_energy;
    int min_cluster_size;
};

#endif
