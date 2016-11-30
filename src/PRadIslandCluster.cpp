//============================================================================//
// PRad Cluster Reconstruction Method                                         //
// Reconstruct the cluster using island algorithm from PrimEx                 //
//                                                                            //
// Ilya Larin, adapted GAMS island algorithm for HyCal in PrimEx.             //
// Weizhi Xiong, adapted island algorithm for PRad, reduced the discretized   //
//               energy value from 10 MeV to 0.1 MeV, wrote a C++ wrapper for //
//               the fortran code. 09/28/2016                                 //
// Chao Peng, rewrote the whole fortran code into C++. 11/21/2016             //
//============================================================================//

#include <cmath>
#include <algorithm>
#include <iostream>
#include "PRadIslandCluster.h"
#include "PRadClusterProfile.h"
#include "PRadHyCalDetector.h"
#include "PRadADCChannel.h"
#include "PRadTDCChannel.h"


PRadIslandCluster::PRadIslandCluster(const std::string &path)
{
    // configuration
    Configure(path);
}

PRadIslandCluster::~PRadIslandCluster()
{
    // place holder
}

PRadHyCalCluster *PRadIslandCluster::Clone()
{
    return new PRadIslandCluster(*this);
}

void PRadIslandCluster::Configure(const std::string &path)
{
    PRadHyCalCluster::Configure(path);

    bool verbose = !path.empty();

    bool corner = getDefConfig<bool>("Corner Connection", false, verbose);
    if(corner)
        adj_dist = 1.6;
    else
        adj_dist = 1.1;

    // set the min module energy for all the module type
    float univ_min_energy = getDefConfig<float>("Min Module Energy", 0., false);
    min_module_energy.resize(PRadHyCalModule::Max_ModuleType, univ_min_energy);

    // update the min module energy if some type is specified
    // the key is "Min Module Energy [typename]"
    for(size_t i = 0; i < min_module_energy.size(); ++i)
    {
        // determine key name
        std::string type = PRadHyCalModule::get_module_type_name(i);
        std::string key = "Min Module Energy [" + type + "]";
        auto value = GetConfigValue(key);
        if(!value.IsEmpty())
            min_module_energy[i] = value.Float();
    }
}

void PRadIslandCluster::FormCluster(std::vector<ModuleHit> &hits,
                                    std::vector<ModuleCluster> &clusters)
const
{
    // clear container first
    clusters.clear();

    // regroup hits in sector
    groupHits(hits, clusters);

    // try to split the energy of shared modules
    splitClusters(clusters);
}

//============================================================================//
// Method based on the code from I. Larin for PrimEx                          //
//============================================================================//
#ifdef PRIMEX_METHOD

void PRadIslandCluster::groupHits(std::vector<ModuleHit> &hits,
                                  std::vector<ModuleCluster> &clusters)
const
{
    // roughly combine all adjacent hits
    for(auto &hit : hits)
    {
        // not belong to any existing cluster
        if(!fillClusters(hit, clusters)) {
            ModuleCluster new_cluster;
            new_cluster.hits.push_back(hit);
            clusters.emplace_back(std::move(new_cluster));
        }
    }

    // merge adjacent cluster
    for(auto it = clusters.begin(); it != clusters.end(); ++it)
    {
        for(auto it_next = it + 1; it_next != clusters.end(); ++it_next)
        {
            if(!checkAdjacent(*it, *it_next))
                continue;

            // merge it to previous cluster and erase it
            it_next->hits.reserve(it->hits.size() + it_next->hits.size());
            it_next->hits.insert(it_next->hits.end(), it->hits.begin(), it->hits.end());
            clusters.erase(it--);
            break;
        }
    }

    // clusters are formed, set the energy center
    for(auto &cluster : clusters)
    {
        const ModuleHit *center = &cluster.center;
        for(auto &hit : cluster.hits)
        {
            cluster.energy += hit.energy;
            if(hit.energy > center->energy)
                center = &hit;
        }
        cluster.center = *center;
    }
}

bool PRadIslandCluster::fillClusters(ModuleHit &hit, std::vector<ModuleCluster> &clusters)
const
{
    for(auto &cluster : clusters)
    {
        for(auto &prev_hit : cluster.hits)
        {
            // it belongs to a existing cluster
            if(GetDistance(hit, prev_hit) < adj_dist) {
                cluster.hits.push_back(hit);
                return true;
            }
        }
    }

    return false;
}

inline bool PRadIslandCluster::checkAdjacent(const ModuleCluster &c1,
                                             const ModuleCluster &c2)
const
{
    for(auto &m1 : c1.hits)
        for(auto &m2 : c2.hits)
            if(GetDistance(m1, m2) < adj_dist)
                return true;

    return false;
}

void PRadIslandCluster::splitClusters(std::vector<ModuleCluster> &)
const
{

}

inline void PRadIslandCluster::splitCluster(ModuleCluster &, ModuleCluster &)
const
{

}

//============================================================================//
// Method based on code from M. Levillain and W. Xiong                        //
//============================================================================//
#else

void PRadIslandCluster::groupHits(std::vector<ModuleHit> &hits,
                                  std::vector<ModuleCluster> &clusters)
const
{
    // sort hits by energy
    std::sort(hits.begin(), hits.end(),
              [] (const ModuleHit &m1, const ModuleHit &m2)
              {
                  return m1.energy > m2.energy;
              });

    // loop over all hits
    for(auto &hit : hits)
    {
        // not belongs to any cluster, and the energy is larger than center threshold
        if(!fillClusters(hit, clusters) && (hit.energy > min_center_energy))
        {
            ModuleCluster new_cluster(hit);
            new_cluster.AddHit(hit);
            clusters.emplace_back(std::move(new_cluster));
        }
    }
}

bool PRadIslandCluster::fillClusters(ModuleHit &hit, std::vector<ModuleCluster> &c)
const
{
    for(auto &cluster : c)
    {
        for(auto &prev_hit : cluster.hits)
        {
            if(GetDistance(hit, prev_hit) < 1.6) {
                cluster.AddHit(hit);
                return true;
            }
        }
    }

    return false;
}

// split energies of the shared modules into two adjacent clusters
// here we assumed the cluster center energy is in a descendant order and the 
// shared modules are always grouped into the cluster with higher center energy
void PRadIslandCluster::splitClusters(std::vector<ModuleCluster> &clusters)
const
{
    for(auto it = clusters.begin(); it != clusters.end(); ++it)
    {
        for(auto it_prev = clusters.begin(); it_prev != it; ++it_prev)
        {
            splitCluster(*it_prev, *it);
        }
    }
}

// it is supposed that c1 has all the possible shared modules
inline void PRadIslandCluster::splitCluster(ModuleCluster &c1, ModuleCluster &c2)
const
{
    std::vector<ModuleHit*> shared_hits;

    for(auto &hit1 : c1.hits)
    {
        for(auto &hit2 : c2.hits)
        {
            if(GetDistance(hit1, hit2) < 1.6) {
                shared_hits.push_back(&hit1);
                break;
            }
        }
    }

    const PRadClusterProfile &profile = PRadClusterProfile::Instance();

    // rough splitting
    for(auto hit : shared_hits)
    {
        // check profile of c1 and c2
        int dx1 = (hit->geo.x - c1.center.geo.x)/c1.center.geo.size_x * 100.;
        int dy1 = (hit->geo.y - c1.center.geo.y)/c1.center.geo.size_y * 100.;
        float frac1 = profile.GetFraction(c1.center.geo.type, abs(dx1), abs(dy1));

        int dx2 = (hit->geo.x - c2.center.geo.x)/c2.center.geo.size_x * 100.;
        int dy2 = (hit->geo.y - c2.center.geo.y)/c2.center.geo.size_y * 100.;
        float frac2 = profile.GetFraction(c2.center.geo.type, abs(dx2), abs(dy2));

        // no need to split
        if(frac2 == 0.)
            continue;

        // calculate the ratio of splitting energy
        float ratio = frac1*c1.center.energy/(frac1*c1.center.energy + frac2*c2.center.energy);

        // copy a new ModuleHit and split its energy
        ModuleHit split_hit(*hit);

        hit->energy *= ratio;
        split_hit.energy -= hit->energy;

        // add hit to c2
        c1.energy -= split_hit.energy;
        c2.AddHit(split_hit);
    }
}
#endif


