//============================================================================//
// Basic PRad Cluster Reconstruction Class For HyCal                          //
// Different reconstruction methods can be implemented accordingly            //
//                                                                            //
// Chao Peng, Weizhi Xiong                                                    //
// 09/28/2016                                                                 //
//============================================================================//

#include <cmath>
#include <iostream>
#include <iomanip>
#include "PRadHyCalCluster.h"

PRadHyCalCluster::PRadHyCalCluster()
: depth_corr(true), log_weight_thres(3.6)
{

}

PRadHyCalCluster::~PRadHyCalCluster()
{
    // place holder
}

PRadHyCalCluster* PRadHyCalCluster::Clone()
{
    return new PRadHyCalCluster(*this);
}

void PRadHyCalCluster::Configure(const std::string &path)
{
    bool verbose = false;

    if(!path.empty()) {
        ConfigObject::Configure(path);
        verbose = true;
    }

    depth_corr = getDefConfig<bool>("Shower Depth Correction", true, verbose);
    log_weight_thres = getDefConfig<float>("Log Weight Threshold", 3.6, verbose);
    min_cluster_energy = getDefConfig<float>("Minimum Cluster Energy", 50., verbose);
    min_center_energy = getDefConfig<float>("Minimum Center Energy", 10., verbose);
    min_cluster_size = getDefConfig<unsigned int>("Minimum Cluster Size", 1, verbose);
}

std::vector<ModuleCluster> PRadHyCalCluster::Reconstruct(std::vector<ModuleHit> &)
const
{
    // to be implemented by methods
    return std::vector<ModuleCluster>();
}

float PRadHyCalCluster::GetWeight(const float &E, const float &E0)
const
{
    float w = log_weight_thres + log(E/E0);
    if(w < 0.)
        return 0.;
    return w;
}

// get shower depth, unit is in MeV
float PRadHyCalCluster::GetShowerDepth(int module_type, const float &E)
const
{
    if(depth_corr && E > 0.) {
        // here all the values are hard coded, because they are all physical
        // values corresponding to the material, so no need to change
        // it returns the maximum shower depth that
        // t = X0*ln(E0/Ec)/ln2, where X0 is radiation length, Ec is critical energy
        // units are in mm and MeV
        if(module_type == PRadHyCalModule::PbWO4)
            return 8.6*log(E/1.1)/log(2.);

        if(module_type == PRadHyCalModule::PbGlass)
            return 26.7*log(E/2.84)/log(2.);
    }

    return 0.;
}

// check if the cluster is good enough
bool PRadHyCalCluster::CheckCluster(const ModuleCluster &cluster)
const
{
    if((cluster.energy < min_cluster_energy) ||
       (cluster.hits.size() < min_cluster_size))
            return false;

    return true;
}

// reconstruct cluster
HyCalHit PRadHyCalCluster::ReconstructHit(const ModuleCluster &cluster)
const
{
    // initialize the hit
    HyCalHit hit(cluster.center.id,         // center id
                 cluster.center.geo.flag,   // module flag
                 cluster.energy);           // total energy

    float weight_x = 0, weight_y = 0, weight_total = 0;

    // count modules
    hit.nblocks = cluster.hits.size();

    // reconstruct position
    for(auto &hit : cluster.hits)
    {
        float weight = GetWeight(hit.energy, cluster.energy);
        weight_x += hit.geo.x*weight;
        weight_y += hit.geo.y*weight;
        weight_total += weight;
    }

    hit.x = weight_x/weight_total;
    hit.y = weight_y/weight_total;

    // z position will need a depth correction, defined in PRadHyCalCluster
    hit.z = cluster.center.geo.z + GetShowerDepth(cluster.center.geo.type, hit.E);

    return hit;
}
