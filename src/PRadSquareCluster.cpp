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

        // merge the center if it belongs to other clusters
        for(auto &cluster : clusters)
        {
            // overlap with other modules, discarded this center
            // TODO, probably better to do splitting other than discarding
            if(checkBelongs(cluster.center, hit, square_size)) {
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
    // group hits into clusters
    for(auto &hit : hits)
    {
        for(auto &cluster : clusters)
        {
            if(checkBelongs(cluster.center, hit, float(square_size)/2.)) {
                cluster.AddHit(hit);
                break;
            }
        }
    }
}

inline bool PRadSquareCluster::checkBelongs(const ModuleHit &center,
                                            const ModuleHit &hit,
                                            float factor)
const
{
    float dist_x = factor*center.geo.size_x;
    float dist_y = factor*center.geo.size_y;

    if((fabs(center.geo.x - hit.geo.x) > dist_x) ||
       (fabs(center.geo.y - hit.geo.y) > dist_y))
        return false;

    return true;
}
