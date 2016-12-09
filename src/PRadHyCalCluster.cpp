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

void PRadHyCalCluster::LeakCorr(ModuleCluster &cluster, const std::vector<ModuleHit> &dead)
const
{
    // no need to do correction
    if(!leak_corr)
        return;

    // prepare variables to be used
    HitInfo temp_hit, cl[POS_RECON_HITS];
    float estimator = 5.; // initial estimator value
    float dead_energy[dead.size()], temp_energy[dead.size()];
    const auto &center = cluster.center;
    for(auto &ene : dead_energy)
    {
        ene = 0.;
    }

    // iteration to correct leakage
    for(unsigned int iter = 0; iter <= leak_iters; ++iter)
    {
        // reconstruct position using cluster hits and dead modules
        if(iter > 0) {
            int count = 0;
            // fill existing cluster hits
            fillHits(cl, count, center, cluster.hits);
            // fill virtual hits for dead modules
            for(size_t i = 0; i < dead.size(); ++i)
            {
                if(dead_energy[i] == 0.)
                    continue;

                const auto &hit = dead.at(i);
                if(PRadHyCalDetector::hit_distance(center, hit) < CORNER_ADJACENT) {
                    cl[count].x = hit.geo.x;
                    cl[count].y = hit.geo.y;
                    cl[count].E = dead_energy[i];
                    count++;
                }
            }
            // reconstruct position
            posRecon(cl, count, temp_hit.x, temp_hit.y);

        } else {
            // first iteration, using center module position only
            temp_hit.x = center.geo.x;
            temp_hit.y = center.geo.y;
        }

        // check profile to determine dead hits' energy
        temp_hit.E = cluster.energy;
        for(size_t i = 0; i < dead.size(); ++i)
        {
            float frac = __hc_prof.GetProfile(temp_hit.x, temp_hit.y, dead.at(i)).frac;

            temp_energy[i] = cluster.energy*frac;
            temp_hit.E += temp_energy[i];
        }

        // check if the correction helps improve the cluster profile
        float new_est = evalEstimator(temp_hit, cluster);
        // improved! record current changes
        if(new_est < estimator) {
            estimator = new_est;
            for(size_t i = 0; i < dead.size(); ++i)
            {
                dead_energy[i] = temp_energy[i];
            }
        // not improved! stop iteration
        } else {
            break;
        }
    }

    // leakage correction to cluster
    for(size_t i = 0; i < dead.size(); ++i)
    {
        // leakage is large enough
        if(dead_energy[i] >= least_leak*cluster.energy) {

            // add virtual hit
            ModuleHit vhit(dead.at(i));
            vhit.energy = dead_energy[i];
            cluster.AddHit(vhit);

            // record leakage correction
            cluster.leakage += vhit.energy;

            // change center if its energy is even larger
            if(vhit.energy > cluster.center.energy) {
                cluster.center = vhit;
            }
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

// evaluate how well the profile can describe this cluster
// tmp is a reconstructed hit that contains position and energy information of
// this cluster
float PRadHyCalCluster::evalEstimator(const HitInfo &tmp, const ModuleCluster &cl)
const
{
    float est = 0.;

    // determine energy resolution
    float res = 0.026;  // 2.6% for PbWO4
    if(TEST_BIT(cl.center.flag, kPbGlass))
        res = 0.065;    // 6.5% for PbGlass
    if(TEST_BIT(cl.center.flag, kTransition))
        res = 0.050;    // 5.0% for transition
    res /= sqrt(tmp.E/1000.);

    for(auto hit : cl.hits)
    {
        const auto &prof = __hc_prof.GetProfile(tmp.x, tmp.y, hit);

        float diff = hit.energy - tmp.E*prof.frac;

        // energy resolution part and profile error part
        float sigma2 = 0.816*hit.energy + res*tmp.E*prof.err;

        // log likelyhood for double exponential distribution
        est += fabs(diff)/sqrt(sigma2);
    }

    return est/cl.hits.size();
}

