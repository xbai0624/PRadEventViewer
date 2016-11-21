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
    sector_hits.resize(PRadHyCalModule::Max_HyCalSector);
    // reserve enough space for each sector
    for(auto &sector : sector_hits)
    {
        sector.reserve(MAX_SECTOR_HITS);
    }

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

    // if no configuration file specified, load the default value quietly
    bool verbose = (!path.empty());

    min_module_energy = getDefConfig<float>("Min Module Energy", 1., verbose);
    if(min_module_energy < 0.) {
        std::cout << "PRad Island Cluster Warning: Min Module Energy cannot be "
                  << "less than 0, automatically changed to 0."
                  << std::endl;
        min_module_energy = 0.;
    }
}

void PRadIslandCluster::Reconstruct(PRadHyCalDetector *det)
{
    if(det == nullptr)
        return;

    // clear the cluster container
    det->hycal_clusters.clear();

    fillSectorHits(det);

    for(auto &sector : sector_hits)
    {
        groupSectorHits(sector);
        splitSectorClusters();
    }
}

void PRadIslandCluster::fillSectorHits(PRadHyCalDetector *det)
{
    // clear sector storage
    for(auto &sector : sector_hits)
    {
        sector.clear();
    }

    // fill sectors
    for(auto &module : det->GetModuleList())
    {
        // too small energy, don't allow it to participate in reconstruction
        if(module->GetEnergy() <= min_module_energy)
            continue;

        auto &sector = sector_hits.at(module->GetSectorID());

        // fill into sector
        sector.emplace_back(module);
    }
}

void PRadIslandCluster::groupSectorHits(const std::vector<PRadHyCalModule*> &sector)
{
    // erase container
    clusters.clear();

    // roughly combine all contiguous hits
    for(auto &module : sector)
    {
        // not belong to any existing cluster
        if(!fillCluster(module)) {
            std::list<PRadHyCalModule*> new_cluster;
            new_cluster.push_back(module);
            clusters.emplace_back(std::move(new_cluster));
        }
    }

    // merge contiguous clusters
    if(clusters.size() > 1) {
        auto it = clusters.begin();
        it++;
        for(; it != clusters.end(); ++it)
        {
            for(auto it_prev = clusters.begin(); it_prev != it; ++it_prev)
            {
                if(!checkContiguous(*it, *it_prev))
                    continue;

                (*it_prev).splice((*it_prev).end(), *it);
                clusters.erase(it);
                it--;
                break;
            }
        }
    }
}

bool PRadIslandCluster::fillCluster(PRadHyCalModule *m)
{
    for(auto &cluster : clusters)
    {
        for(auto &prev_m : cluster)
        {
            // it belongs to a existing cluster
            if(checkContiguous(m, prev_m)) {
                cluster.push_back(m);
                return true;
            }
        }
    }

    return false;
}

inline bool PRadIslandCluster::checkContiguous(const PRadHyCalModule* m1,
                                               const PRadHyCalModule* m2)
const
{
    int diff =  abs(m1->GetColumn() - m2->GetColumn())
              + abs(m1->GetRow() - m2->GetRow());
    if(diff < 2)
        return true;
    else
        return false;
}

inline bool PRadIslandCluster::checkContiguous(const std::list<PRadHyCalModule*> &c1,
                                               const std::list<PRadHyCalModule*> &c2)
const
{
    for(auto &m1 : c1)
        for(auto &m2 : c2)
            if(checkContiguous(m1, m2))
                return true;

    return false;
}

void PRadIslandCluster::splitSectorClusters()
{

}
