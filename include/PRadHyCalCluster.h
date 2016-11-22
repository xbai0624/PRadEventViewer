#ifndef PRAD_HYCAL_CLUSTER_H
#define PRAD_HYCAL_CLUSTER_H

#include <string>
#include <iostream>
#include "ConfigObject.h"
#include "PRadEventStruct.h"
#include "PRadHyCalDetector.h"

    struct ModuleHit
    {
        int id;                         // module id
        PRadHyCalModule::Geometry geo;  // geometry
        float energy;                   // participated energy, may be splitted

        ModuleHit() : id(0), energy(0) {};
        ModuleHit(int i, const PRadHyCalModule::Geometry &g, float e)
        : id(i), geo(g), energy(e)
        {};
    };

    struct ModuleCluster
    {
        ModuleHit center;
        std::vector<ModuleHit> hits;    // hits group
        float energy;

        ModuleCluster(const ModuleHit &hit) : center(hit), energy(0) {};

        void AddHit(const ModuleHit &hit)
        {
            hits.emplace_back(hit);
            energy += hit.energy;
        }
    };
class PRadHyCalCluster : public ConfigObject
{
public:
    virtual ~PRadHyCalCluster();
    virtual PRadHyCalCluster *Clone();
    virtual void Configure(const std::string &path);
    virtual void Reconstruct(PRadHyCalDetector *det);
    float GetWeight(const float &E, const float &E0);
    float GetShowerDepth(int module_type, const float &E);
    void NonLinearCorr(PRadHyCalModule *center, float &E);
    bool CheckCluster(const HyCalCluster &hit);
    HyCalCluster Reconstruct(const ModuleCluster &cluster);

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
