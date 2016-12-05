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

const PRadClusterProfile &profile = PRadClusterProfile::Instance();

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
        adj_dist = CORNER_ADJACENT;
    else
        adj_dist = SIDE_ADJACENT;

    // set the min module energy for all the module type
    float univ_min_energy = getDefConfig<float>("Min Module Energy", 0., false);
    min_module_energy.resize(PRadHyCalModule::Max_ModuleType, univ_min_energy);

    // update the min module energy if some type is specified
    // the key is "Min Module Energy [typename]"
    for(unsigned int i = 0; i < min_module_energy.size(); ++i)
    {
        // determine key name
        std::string type = PRadHyCalModule::get_module_type_name(i);
        std::string key = "Min Module Energy [" + type + "]";
        auto value = GetConfigValue(key);
        if(!value.IsEmpty())
            min_module_energy[i] = value.Float();
    }
}

//============================================================================//
// Method based on the code from I. Larin for PrimEx                          //
//============================================================================//
#ifdef PRIMEX_METHOD

void PRadIslandCluster::FormCluster(std::vector<ModuleHit> &hits,
                                    std::vector<ModuleCluster> &clusters)
const
{
    // clear container first
    clusters.clear();

    std::list<std::list<ModuleHit*>> groups;

    // group adjacent hits
    groupHits(hits, groups);

    // try to split the group
    for(auto &group : groups)
    {
        splitCluster(group, clusters);
    }
}

void PRadIslandCluster::groupHits(std::vector<ModuleHit> &hits,
                                  std::list<std::list<ModuleHit*>> &hits_groups)
const
{
    // roughly combine all adjacent hits
    for(auto &hit : hits)
    {
        if(hit.energy < min_module_energy.at(hit.geo.type))
            continue;

        // not belong to any existing cluster
        if(!fillClusters(hit, hits_groups)) {
            std::list<ModuleHit*> new_group;
            new_group.push_back(&hit);
            hits_groups.emplace_back(std::move(new_group));
        }
    }

    // merge adjacent groups
    for(auto it = hits_groups.begin(); it != hits_groups.end(); ++it)
    {
        auto it_next = it;
        while(++it_next != hits_groups.end())
        {
            if(checkAdjacent(*it, *it_next)) {
                it_next->splice(it_next->end(), *it);
                hits_groups.erase(it--);
                break;
            }
        }
    }
}

bool PRadIslandCluster::fillClusters(ModuleHit &hit, std::list<std::list<ModuleHit*>> &groups)
const
{
    for(auto &group : groups)
    {
        for(auto &prev_hit : group)
        {
            // it belongs to a existing cluster
            if(GetDistance(hit, *prev_hit) < adj_dist) {
                group.push_back(&hit);
                return true;
            }
        }
    }

    return false;
}

inline bool PRadIslandCluster::checkAdjacent(const std::list<ModuleHit*> &g1,
                                             const std::list<ModuleHit*> &g2)
const
{
    for(auto &m1 : g1)
    {
        for(auto &m2 : g2)
        {
            if(GetDistance(*m1, *m2) < adj_dist) {
                return true;
            }
        }
    }

    return false;
}

// split one group into several clusters
void PRadIslandCluster::splitCluster(std::list<ModuleHit*> &group,
                                     std::vector<ModuleCluster> &clusters)
const
{
    // find local maximum
    std::vector<ModuleHit*> local_max;
    for(auto &hit1 : group)
    {
        if(hit1->energy < min_center_energy)
            continue;

        bool maximum = true;
        for(auto &hit2 : group)
        {
            // we count corner in
            if((GetDistance(*hit1, *hit2) < CORNER_ADJACENT) &&
               (hit2->energy > hit1->energy)) {
                maximum = false;
                break;
            }
        }

        if(maximum)
            local_max.push_back(hit1);
    }

    // no need to split
    if(local_max.empty())
        return;

    if(local_max.size()) {// == 1) {
        ModuleCluster new_cluster(*local_max.front());
        for(auto &hit : group)
            new_cluster.AddHit(*hit);
        clusters.emplace_back(std::move(new_cluster));
    }

    // prepare clusters for split
    std::vector<ModuleCluster> split_clusters;
    for(auto &center : local_max)
    {
        split_clusters.emplace_back(*center);
        split_clusters.back().AddHit(*center);
    }

    // do iteration to refine the splitting
    int split_iter = 6;
    for(int i = 0; i < split_iter; ++i)
    {

    }
}
/* don't split, output the full hits group
{
    ModuleCluster new_cluster;
    ModuleHit *center = &new_cluster.center;
    for(auto &hit : group)
    {
        new_cluster.AddHit(*hit);
        if(hit->energy > center->energy)
            center = hit;
    }

    if(center->energy > min_center_energy) {
        new_cluster.center = *center;
        clusters.emplace_back(std::move(new_cluster));
    }
}
*/

//============================================================================//
// Method based on code from M. Levillain and W. Xiong                        //
//============================================================================//
#else

void PRadIslandCluster::FormCluster(std::vector<ModuleHit> &hits,
                                    std::vector<ModuleCluster> &clusters)
const
{
    // clear container first
    clusters.clear();

    // form clusters with high energy hit seed
    groupHits(hits, clusters);
}

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
        // less than min module energy, ignore this hit
        if(hit.energy < min_module_energy.at(hit.geo.type))
            continue;

        // not belongs to any cluster, and the energy is larger than center threshold
        if(!fillClusters(hit, clusters) && (hit.energy > min_center_energy))
        {
            clusters.emplace_back(hit);
            clusters.back().AddHit(hit);
        }
    }
}

bool PRadIslandCluster::fillClusters(ModuleHit &hit, std::vector<ModuleCluster> &c)
const
{
    std::vector<unsigned int> indices;
    indices.reserve(5);

    for(unsigned int i = 0; i < c.size(); ++i)
    {
        for(auto &prev_hit : c.at(i).hits)
        {
            if(GetDistance(hit, prev_hit) < CORNER_ADJACENT) {
                indices.push_back(i);
                break;
            }
        }
    }

    // it belongs to no cluster
    if(indices.empty())
        return false;

    // it belongs to single cluster
    if(indices.size() == 1) {
        c.at(indices.front()).AddHit(hit);
        return true;
    }

    // it belongs to several clusters
    return splitHit(hit, c, indices);
}

// split hit that belongs to several clusters
bool PRadIslandCluster::splitHit(ModuleHit &hit,
                                 std::vector<ModuleCluster> &clusters,
                                 std::vector<unsigned int> &indices)
const
{
    // energy fraction
    std::vector<float> frac(indices.size(), 0.);
    float total_frac = 0.;

    // rough splitting, only use the center position
    // to refine it, use a reconstruction position or position from other detector
    for(unsigned int i = 0; i < indices.size(); ++i)
    {
        auto &center = clusters.at(indices.at(i)).center;
        // discretize the distance to 1/100 of the cell size
        int dx = abs((hit.geo.x - center.geo.x)/center.geo.size_x * 100.);
        int dy = abs((hit.geo.y - center.geo.y)/center.geo.size_y * 100.);
        frac.at(i) = profile.GetFraction(center.geo.type, dx, dy);
        total_frac += frac.at(i);
    }

    // this hit is too far away from all clusters, discard it
    if(total_frac == 0.) {
        return false;
    }

    // add hit to all clusters
    for(unsigned int i = 0; i < indices.size(); ++i)
    {
        // this cluster has no share of the hit
        if(frac.at(i) == 0.)
            continue;

        auto &cluster = clusters.at(indices.at(i));
        ModuleHit shared_hit(hit);
        shared_hit.energy *= frac.at(i)/total_frac;
        cluster.AddHit(shared_hit);
    }

    return true;
}

#endif


