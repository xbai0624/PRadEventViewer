//============================================================================//
// PRad Cluster Reconstruction Method                                         //
// Reconstruct the cluster within a square area                               //
//                                                                            //
// Weizhi Xiong, Chao Peng                                                    //
// 06/10/2016                                                                 //
//============================================================================//

#include <cmath>
#include <list>
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

void PRadSquareCluster::FormCluster(std::vector<ModuleHit> &hits,
                                    std::vector<ModuleCluster> &clusters)
const
{
    // clear container first
    clusters.clear();

    // create new cluster with its center
    findCenters(hits, clusters);

    // fill the clusters by square shape
    fillClusters(hits, clusters);
}

void PRadSquareCluster::findCenters(std::vector<ModuleHit> &hits,
                                    std::vector<ModuleCluster> &clusters)
const
{
    // searching possible cluster centers
    for(auto &hit : hits)
    {
        // did not pass the requirement on least energy for cluster center
        if(hit.energy < min_center_energy)
            continue;

        bool overlap = false;
        float dist_x = float(square_size)*hit.geo.size_x;
        float dist_y = float(square_size)*hit.geo.size_y;

        // merge the center if it belongs to other clusters
        for(auto &cluster : clusters)
        {
            // overlap with other modules, discarded this center
            // TODO, probably better to do splitting other than discarding
            if((fabs(cluster.center.geo.x - hit.geo.x) <= dist_x) &&
               (fabs(cluster.center.geo.y - hit.geo.y) <= dist_y)) {
                overlap = true;

                // change this center if it has less energy
                if(cluster.center.energy < hit.energy) {
                    cluster.center = hit;
                }
                break;
            }
        }

        // a new center found
        if(!overlap) {
            clusters.emplace_back(hit);
        }
    }
}

void PRadSquareCluster::fillClusters(std::vector<ModuleHit> &hits,
                                     std::vector<ModuleCluster> &clusters)
const
{
    // claiming territory for the cluster
    for(auto &cluster : clusters)
    {
        float dist_x = float(square_size)*cluster.center.geo.size_x/2.;
        float dist_y = float(square_size)*cluster.center.geo.size_y/2.;
        for(auto it = hits.begin(); it != hits.end(); ++it)
        {
            // not belongs to the cluster
            if((fabs(cluster.center.geo.x - (*it).geo.x) > dist_x) ||
               (fabs(cluster.center.geo.y - (*it).geo.y) > dist_y))
                continue;
            cluster.AddHit(*it);
            hits.erase(it--);
        }
    }
}
