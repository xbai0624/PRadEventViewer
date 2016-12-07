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

void PRadHyCalCluster::FormCluster(std::vector<ModuleHit> &,
                                   std::vector<ModuleCluster> &)
const
{
    // to be implemented by methods
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
        // here all the values are hard coded, because these are all physical
        // values corresponding to the material, so no need to change
        // it returns the maximum shower depth that
        // t = X0*ln(E0/Ec)/ln2, where X0 is radiation length, Ec is critical energy
        // units are in mm and MeV
        if(module_type == PRadHyCalModule::PbWO4)
            return 8.6*log(E/1.1)/log(2.);

        // -101.2 is the surface difference between Lead Glass and Lead Tungstate modules
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

// a global container to have a better performance
float __hc_cl_x[POS_RECON_HITS], __hc_cl_y[POS_RECON_HITS], __hc_cl_E[POS_RECON_HITS];

// reconstruct cluster
HyCalHit PRadHyCalCluster::Reconstruct(const ModuleCluster &cluster)
const
{
    // initialize the hit
    HyCalHit hycal_hit(cluster.center.id,       // center id
                       cluster.center.flag,     // module flag
                       cluster.energy);         // total energy

    // count modules
    hycal_hit.nblocks = cluster.hits.size();

    // reconstruct position
    // only use the center 3x3 to reconstruct the module position
    int count = 0;
    float energy = 0;
    float weight_x = 0, weight_y = 0, weight_total = 0;

    for(auto &hit : cluster.hits)
    {
        if(PRadHyCalDetector::hit_distance(cluster.center, hit) < 1.6) {
            __hc_cl_x[count] = hit.geo.x;
            __hc_cl_y[count] = hit.geo.y;
            __hc_cl_E[count] = hit.energy;
            energy += hit.energy;
            count++;
        }
    }

    for(int i = 0; i < count; ++i)
    {
        float weight = GetWeight(__hc_cl_E[i], energy);
        weight_x += __hc_cl_x[i]*weight;
        weight_y += __hc_cl_y[i]*weight;
        weight_total += weight;
    }

    /* all hits participate in position reconstruction
    for(auto &hit : cluster.hits)
    {
        float weight = GetWeight(hit.energy, cluster.energy);
        weight_x += hit.geo.x*weight;
        weight_y += hit.geo.y*weight;
        weight_total += weight;
    }
    */

    hycal_hit.x = weight_x/weight_total;
    hycal_hit.y = weight_y/weight_total;
    hycal_hit.z = cluster.center.geo.z;

    // z position will need a depth correction
    hycal_hit.z += GetShowerDepth(cluster.center.geo.type, cluster.energy);

    return hycal_hit;
}
