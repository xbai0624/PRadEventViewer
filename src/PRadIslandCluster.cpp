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
#include <list>
#include <algorithm>
#include <iostream>
#include "PRadIslandCluster.h"
#include "PRadHyCalDetector.h"
#include "PRadADCChannel.h"
#include "PRadTDCChannel.h"

// reserve space to store sector hits
#define MAX_SECTOR_HITS 1000
// to separate different col, increase if there are more modules in a column
#define COL_SEP 100



PRadIslandCluster::PRadIslandCluster(const std::string &path)
{
    // a container to store the hits by sector
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
        float energy = module->GetEnergy();

        // too small energy, don't allow it to participate in reconstruction
        if(energy <= min_module_energy)
            continue;

        // fill into sector
        auto &sector = sector_hits.at(module->GetSectorID());
        int sector_addr = module->GetColumn()*COL_SEP + module->GetRow();
        sector.emplace_back(module, sector_addr, energy);
    }

    // sort sectors
    for(auto &sector: sector_hits)
    {
        std::sort(sector.begin(), sector.end());
    }
}

void PRadIslandCluster::groupSectorHits(const std::vector<ModuleHit> &sector)
{
    if(sector.empty())
        return;

    std::list<std::list<ModuleHit>> clusters;
    std::list<ModuleHit> this_cluster;

    this_cluster.push_back(sector.at(0));

    for(size_t i = 1; i < sector.size(); ++i)
    {
        // contiguous modules in a column
        if(sector.at(i) - sector.at(i-1) <= 1) {
            this_cluster.push_back(sector.at(i));
        } else {
            clusters.emplace_back(std::move(this_cluster));
            this_cluster.clear();
        }
    }
}

