//============================================================================//
// PRad Cluster Reconstruction Method                                         //
// Reconstruct the cluster within a square area                               //
//                                                                            //
// Weizhi Xiong, Chao Peng                                                    //
// 06/10/2016                                                                 //
//============================================================================//

#include <cmath>
#include "PRadSquareCluster.h"
#include "PRadHyCalDetector.h"
#include "PRadADCChannel.h"
#include "PRadTDCChannel.h"

PRadSquareCluster::PRadSquareCluster(const std::string &path)
{
    Configure(path);
}

PRadSquareCluster::~PRadSquareCluster()
{
    // place holder
}

PRadHyCalCluster *PRadSquareCluster::Clone()
{
    return new PRadSquareCluster(*this);
}

void PRadSquareCluster::Configure(const std::string &path)
{
    PRadHyCalCluster::Configure(path);

    // if no configuration file specified, load the default value quietly
    bool verbose = (!path.empty());

    square_size = getDefConfig<unsigned int>("Square Size", 5, verbose);
}

void PRadSquareCluster::Reconstruct(PRadHyCalDetector *det)
{
    if(det == nullptr)
        return;

    // clear the cluster container
    det->hycal_clusters.clear();

    auto centers = findCenters(det);

    for(auto &center : centers)
    {
        HyCalHit hit = formCluster(center, det);

        // if the cluster is good
        if(CheckCluster(hit))
            det->hycal_clusters.emplace_back(std::move(hit));
    }
}

std::vector<PRadHyCalModule*> PRadSquareCluster::findCenters(const PRadHyCalDetector *det)
{
    // searching possible cluster centers
    std::vector<PRadHyCalModule*> center_modules;
    for(auto &module : det->GetModuleList())
    {
        // did not pass the requirement on least energy for cluster center
        if(module->GetEnergy() < min_center_energy)
            continue;

        bool overlap = false;
        float dist_x = float(square_size)*module->GetSizeX();
        float dist_y = float(square_size)*module->GetSizeY();

        for(auto &center : center_modules)
        {
            // overlap with other modules, discarded this center
            // TODO, probably better to do splitting other than discarding
            if((fabs(center->GetX() - module->GetX()) <= dist_x) &&
               (fabs(center->GetY() - module->GetY()) <= dist_y)) {
                overlap = true;

                // change this center if it has less energy
                if(center->GetEnergy() < module->GetEnergy()) {
                    center = module;
                }
                break; // no need to continue the check
            }
        }

        if(!overlap)
            center_modules.push_back(module);
    }

    return center_modules;
}

HyCalHit PRadSquareCluster::formCluster(PRadHyCalModule *center, const PRadHyCalDetector *det)
{
    // initialize the hit
    HyCalHit hit(det->GetDetID(),                       // det id
                 center->GetID(),                       // center id
                 center->GetGeometryFlag(),             // module flag
                 center->GetTDC()->GetTimeMeasure());   // timing

    std::vector<PRadHyCalModule *> group;

    float dist_x = float(square_size)/2.*center->GetSizeX();
    float dist_y = float(square_size)/2.*center->GetSizeY();

    // search the modules and group them
    for(auto &module : det->GetModuleList())
    {
        // not within the cluster
        if((fabs(center->GetX() - module->GetX()) > dist_x) ||
           (fabs(center->GetY() - module->GetY()) > dist_y))
            continue;

        hit.E += module->GetEnergy();                   // accumulate energy
        group.push_back(module);
    }

    // count modules
    hit.nblocks = group.size();

    float weight_x = 0, weight_y = 0, weight_total = 0;

    // reconstruct position
    for(auto &module : group)
    {
        float weight = GetWeight(module->GetEnergy(), hit.E);
        weight_x += module->GetX()*weight;
        weight_y += module->GetY()*weight;
        weight_total += weight;
    }

    hit.x = weight_x/weight_total;
    hit.y = weight_y/weight_total;

    // z position will need a depth correction, defined in PRadHyCalCluster
    hit.z = center->GetZ() + GetShowerDepth(center->GetType(), hit.E);
    // correct the non-linear response to the energy, defined in PRadHyCalCluster
    NonLinearCorr(center, hit.E);

    return hit;
}

