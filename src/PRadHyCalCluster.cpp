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
#include "PRadClusterProfile.h"


const PRadClusterProfile &__hc_prof = PRadClusterProfile::Instance();

PRadHyCalCluster::PRadHyCalCluster()
: depth_corr(true), leak_corr(true), linear_corr(true),
  log_weight_thres(3.6), min_cluster_energy(30.), min_center_energy(10.),
  least_leak(0.05), linear_corr_limit(0.6), min_cluster_size(1), leak_iters(3)
{
    // place holder
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
    leak_corr = getDefConfig<bool>("Leakage Correction", true, verbose);
    linear_corr = getDefConfig<bool>("Non Linearity Correction", true, verbose);
    log_weight_thres = getDefConfig<float>("Log Weight Threshold", 3.6, verbose);
    min_cluster_energy = getDefConfig<float>("Minimum Cluster Energy", 50., verbose);
    min_center_energy = getDefConfig<float>("Minimum Center Energy", 10., verbose);
    min_cluster_size = getDefConfig<unsigned int>("Minimum Cluster Size", 1, verbose);
    least_leak = getDefConfig<float>("Least Leakage Fraction", 0.05, verbose);
    leak_iters = getDefConfig<unsigned int>("Leakage Iterations", 3, verbose);
    linear_corr_limit = getDefConfig<float>("Non Linearity Limit", 0.6, verbose);
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

// reconstruct cluster
HyCalHit PRadHyCalCluster::Reconstruct(const ModuleCluster &cluster, const float &alpE)
const
{
    // initialize the hit
    HyCalHit hycal_hit(cluster.center.id,       // center id
                       cluster.center.flag,     // module flag
                       cluster.energy,          // total energy
                       cluster.leakage);        // energy from leakage corr

    // do non-linearity energy correction
    if(linear_corr && fabs(alpE) < linear_corr_limit) {
        float corr = 1./(1 + alpE);
        // save the correction factor, not alpha(E)
        hycal_hit.lin_corr = corr;
        hycal_hit.E *= corr;
    }

    // count modules
    hycal_hit.nblocks = cluster.hits.size();

    // fill 3x3 hits around center into temp container for position reconstruction
    int count = 0;
    HitInfo cl[POS_RECON_HITS];
    fillHits(cl, count, cluster.center, cluster.hits);

    // record how many hits participated in position reconstruction
    hycal_hit.npos = count;

    // reconstruct position
    posRecon(cl, count, hycal_hit.x, hycal_hit.y);
    hycal_hit.z = cluster.center.geo.z;

    // z position will need a depth correction
    hycal_hit.z += GetShowerDepth(cluster.center.geo.type, cluster.energy);

    return hycal_hit;
}

void PRadHyCalCluster::LeakCorr(ModuleCluster &cluster, std::vector<ModuleHit> &dead)
const
{
    // no need to do correction
    if(!leak_corr)
        return;

    // erase dead module energies first
    for(auto &hit : dead)
    {
        hit.energy = 0.;
    }

    for(unsigned int iter = 0; iter <= leak_iters; ++iter)
    {
        float x, y;
        // reconstruct position using cluster hits and dead modules
        if(iter > 0) {
            int count = 0;
            HitInfo cl[POS_RECON_HITS];
            fillHits(cl, count, cluster.center, cluster.hits);
            fillHits(cl, count, cluster.center, dead);
            posRecon(cl, count, x, y);
        } else {
            // first iteration, using center module position only
            x = cluster.center.geo.x;
            y = cluster.center.geo.y;
        }

        for(auto &hit : dead)
        {
            float frac = __hc_prof.GetProfile(x, y, hit).frac;

            hit.energy = cluster.energy*frac;
        }
    }

    for(auto &hit : dead)
    {
        // leakage is too small, don't correct it
        if(hit.energy > least_leak*cluster.energy) {
            cluster.AddHit(hit);
            cluster.leakage += hit.energy;
        }
    }
}

// only use the center 3x3 to fill the temp container
inline void PRadHyCalCluster::fillHits(HitInfo *temp, int &count,
                                       const ModuleHit &center,
                                       const std::vector<ModuleHit> &hits)
const
{
    for(auto &hit : hits)
    {
        if(hit.energy == 0.)
            continue;

        if(PRadHyCalDetector::hit_distance(center, hit) < CORNER_ADJACENT) {
            temp[count].x = hit.geo.x;
            temp[count].y = hit.geo.y;
            temp[count].E = hit.energy;
            count++;
        }
    }
}

// reconstruct position from the temp container
inline void PRadHyCalCluster::posRecon(HitInfo *temp, int count, float &x, float &y)
const
{
    // get total energy
    float energy = 0;
    for(int i= 0; i < count; ++i)
    {
        energy += temp[i].E;
    }

    // reconstruct position
    float weight_x = 0, weight_y = 0, weight_total = 0;
    for(int i = 0; i < count; ++i)
    {
        float weight = GetWeight(temp[i].E, energy);
        weight_x += temp[i].x*weight;
        weight_y += temp[i].y*weight;
        weight_total += weight;
    }

    x = weight_x/weight_total;
    y = weight_y/weight_total;
}
