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
: depth_corr(true), non_linear_corr(true), log_weight_thres(3.6)
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
    non_linear_corr = getDefConfig<bool>("Non Linear Energy Correction", true, verbose);
    log_weight_thres = getDefConfig<float>("Log Weight Threshold", 3.6, verbose);
    min_cluster_energy = getDefConfig<float>("Minimum Cluster Energy", 50., verbose);
    min_center_energy = getDefConfig<float>("Minimum Center Energy", 10., verbose);
    min_cluster_size = getDefConfig<int>("Minimum Cluster Size", 1, verbose);
}

void PRadHyCalCluster::Reconstruct(PRadHyCalDetector * /*det*/)
{
    // to be implemented by methods
}

float PRadHyCalCluster::GetWeight(const float &E, const float &E0)
{
    float w = log_weight_thres + log(E/E0);
    if(w < 0.)
        return 0.;
    return w;
}

// get shower depth, unit is in MeV
float PRadHyCalCluster::GetShowerDepth(int module_type, const float &E)
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

// correct the non linear energy response in HyCal, unit is in MeV
void PRadHyCalCluster::NonLinearCorr(PRadHyCalModule *center, float &E)
{
    if(!non_linear_corr)
        return;

    float alpha = center->GetNonLinearConst();
    float Ecal = center->GetCalibrationEnergy();
    float ecorr = 1. + alpha*(E - Ecal)/1000.;

    // prevent unreasonably correction
    if(fabs(ecorr - 1.) < 0.6)
        E /= ecorr;
}

// check if the cluster is good enough
bool PRadHyCalCluster::CheckCluster(const HyCalCluster &hit)
{
    if((hit.E < min_cluster_energy) ||
       (hit.nblocks < min_cluster_size))
            return false;

    return true;
}

// reconstruct cluster
HyCalCluster PRadHyCalCluster::Reconstruct(const ModuleCluster &cluster)
{
    // initialize the hit
    HyCalCluster hit(cluster.center.id,         // center id
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
    // correct the non-linear response to the energy, defined in PRadHyCalCluster
    //NonLinearCorr(center, hit.E);

    return hit;
}
