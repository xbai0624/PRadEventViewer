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
    // sort hits by their energy
    std::sort(hits.begin(), hits.end(),
              [] (const ModuleHit &m1, const ModuleHit &m2)
              {
                  return m1.energy > m2.energy;
              });

    // the cluster starts with a seed hit, and grows if there are adjacent its
    // clustered hits will be removed, so the for loop needs to be this way
    for(size_t i = 0; i < hits.size(); ++i)
    {
        ModuleCluster new_cluster(hits.at(i));
        new_cluster.AddHit(hits.at(i));
        hits.erase(hits.begin() + i);

        // loop over the cluster until no new hit found
        while(fillCluster(new_cluster, hits))
        {
            ;
        }
        clusters.emplace_back(std::move(new_cluster));
    }
}

bool PRadIslandCluster::fillCluster(ModuleCluster &c, std::vector<ModuleHit> &hits)
const
{
    bool changed = false;

    for(size_t i = 0; i < hits.size(); ++i)
    {
        for(auto &prev_hit : c.hits)
        {
            if(checkContiguous(hits.at(i), prev_hit)) {
                c.AddHit(hits.at(i));
                hits.erase(hits.begin() + i);
                changed = true;
                break;
            }
        }
    }

    return changed;
}

inline bool PRadIslandCluster::checkContiguous(const ModuleHit &m1, const ModuleHit &m2)
const
{
    double dist_x = std::abs(m1.geo.x - m2.geo.x);
    double dist_y = std::abs(m1.geo.y - m2.geo.y);

    if((dist_x > (m1.geo.size_x + m2.geo.size_x)/2.) ||
      (dist_y > (m1.geo.size_y + m2.geo.size_y)/2.))
        return false;

    return true;
}

void PRadIslandCluster::splitSectorClusters()
{

}
