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
#include "PRadHyCalDetector.h"
#include "PRadADCChannel.h"
#include "PRadTDCChannel.h"

// reserve space to store sector hits
#define MAX_SECTOR_HITS 1000



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
    groupSectorHits(hits, clusters);
}

void PRadIslandCluster::groupSectorHits(std::vector<ModuleHit> &hits,
                                        std::vector<ModuleCluster> &clusters)
const
{
    // roughly combine all contiguous hits
    for(auto &hit : hits)
    {
        // not belong to any existing cluster
        if(!fillCluster(hit, clusters)) {
            ModuleCluster new_cluster;
            new_cluster.AddHit(hit);
            clusters.emplace_back(std::move(new_cluster));
        }
    }

    // merge contiguous clusters
    for(size_t i = 1; i < clusters.size(); ++i)
    {
        for(size_t j = 0; j < i; ++j)
        {
            // not contiguous
            if(!checkContiguous(clusters.at(i), clusters.at(j)))
                continue;

            // merge it to previous cluster and erase it
            clusters.at(j).Merge(clusters.at(i));
            clusters.erase(clusters.begin() + i);
            i--;
            break;
        }
    }

    // clusters are formed, set the energy center
    for(auto &cluster : clusters)
    {
        size_t c_hit = 0;
        float energy = 0;
        for(size_t i = 0; i < cluster.hits.size(); ++i)
        {
            if(energy < cluster.hits.at(i).energy) {
                energy = cluster.hits.at(i).energy;
                c_hit = i;
            }
        }
        cluster.center = cluster.hits.at(c_hit);
    }
}

bool PRadIslandCluster::fillCluster(ModuleHit &hit, std::vector<ModuleCluster> &clusters)
const
{
    for(auto &cluster : clusters)
    {
        for(auto &prev_hit : cluster.hits)
        {
            // it belongs to a existing cluster
            if(checkContiguous(hit, prev_hit)) {
                cluster.AddHit(hit);
                return true;
            }
        }
    }

    return false;
}

inline bool PRadIslandCluster::checkContiguous(const ModuleHit &m1, const ModuleHit &m2)
const
{
    if(m1.geo.sector != m2.geo.sector)
        return false;

    int diff = abs(m1.geo.column - m2.geo.column) + abs(m1.geo.row - m2.geo.row);

    if(diff < 2)
        return true;
    else
        return false;
}

inline bool PRadIslandCluster::checkContiguous(const ModuleCluster &c1,
                                               const ModuleCluster &c2)
const
{
    for(auto &m1 : c1.hits)
        for(auto &m2 : c2.hits)
            if(checkContiguous(m1, m2))
                return true;

    return false;
}

void PRadIslandCluster::splitSectorClusters()
{

}
