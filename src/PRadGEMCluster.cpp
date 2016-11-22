//============================================================================//
// Basic PRad Cluster Reconstruction Class For GEM                            //
// GEM Planes send hits infromation and container for the GEM Clusters to this//
// class reconstruct the clusters and send it back to GEM Planes.             //
// Thus the clustering algorithm can be adjusted in this class.               //
//                                                                            //
// Xinzhan Bai & Kondo Gnanvo, first version coding of the algorithm          //
// Chao Peng, adapted to PRad analysis software package                       //
// 10/21/2016                                                                 //
//============================================================================//

#include <iostream>
#include <iomanip>
#include <algorithm>
#include "PRadGEMCluster.h"
#include "PRadGEMDetector.h"

// constructor
PRadGEMCluster::PRadGEMCluster(const std::string &config_path)
{
    Configure(config_path);
}

// destructor
PRadGEMCluster::~PRadGEMCluster()
{
    // place holder
}

// configure the cluster method
void PRadGEMCluster::Configure(const std::string &path)
{
    // if no configuration file specified, load the default value quietly
    bool verbose = false;

    if(!path.empty()) {
        ConfigObject::Configure(path);
        verbose = true;
    }

    min_cluster_hits = getDefConfig<unsigned int>("Min Cluster Hits", 1, verbose);
    max_cluster_hits = getDefConfig<unsigned int>("Max Cluster Hits", 20, verbose);
    split_cluster_diff = getDefConfig<float>("Split Threshold", 14, verbose);
}

// reconstruct, accepts GEM detector
void PRadGEMCluster::Reconstruct(PRadGEMDetector *det)
{
    for(auto &plane : det->GetPlaneList())
        Reconstruct(plane);

    FormClusters(det);
}

// reconstruct accepts GEM plane
void PRadGEMCluster::Reconstruct(PRadGEMPlane *plane)
{
    auto &cluster_list = plane->GetPlaneClusters();

    // clear existing clusters
    cluster_list.clear();

    auto &hits_list = plane->GetPlaneHits();
    // group consecutive hits as the preliminary clusters
    clusterHits(hits_list, cluster_list);

    // split the clusters that may contain multiple physical hits
    splitCluster(cluster_list);

    // remove the clusters that does not pass certain criteria
    filterCluster(cluster_list);

    // reconstruct position and charge for the cluster
    reconstructCluster(cluster_list, plane);
}

// group consecutive hits
void PRadGEMCluster::clusterHits(std::vector<GEMPlaneHit> &hit_list,
                                 std::list<GEMPlaneCluster> &cluster_list)
{
    // sort the hits by its strip number
    std::sort(hit_list.begin(), hit_list.end(),
              // lamda expr, compare hit by their strip numbers
              [](const GEMPlaneHit &h1, const GEMPlaneHit &h2)
              {
                  return h1.strip < h2.strip;
              });

    // group the hits that have consecutive strip number
    auto cluster_begin = hit_list.begin();
    for(auto it = hit_list.begin(); it != hit_list.end(); ++it)
    {
        auto next = it + 1;

        // end of list, group the last cluster
        if(next == hit_list.end()) {
            cluster_list.emplace_back(std::vector<GEMPlaneHit>(cluster_begin, next));
            break;
        }

        // check consecutivity
        if(next->strip - it->strip > 1) {
            cluster_list.emplace_back(std::vector<GEMPlaneHit>(cluster_begin, next));
            cluster_begin = next;
        }
    }
}

// split cluster at valley
void PRadGEMCluster::splitCluster(std::list<GEMPlaneCluster> &cluster_list)
{
    // We are trying to find the valley that satisfies certain critieria,
    // i.e., less than a sigma comparing to neighbor strips on both sides.
    // Such valley probably means two clusters are grouped together, so it
    // will be separated, and each gets 1/2 of the charge from the overlap
    // strip.

    // loop over the cluster list
    for(auto c_it = cluster_list.begin(); c_it != cluster_list.end(); ++c_it)
    {
        // no need to do separation if less than 3 hits
        if(c_it->hits.size() < 3)
            continue;

        // new cluster for the latter part after split
        GEMPlaneCluster split_cluster;

        // insert the splited cluster if there is one
        if(splitCluster_sub(*c_it, split_cluster))
            cluster_list.insert(std::next(c_it, 1), split_cluster);
    }
}

// This function helps splitCluster
// It finds the FIRST local minimum, separate the cluster at its position
// The charge at local minimum strip will be halved, and kept for both original
// and split clusters.
// It returns true if a cluster is split, and vice versa
// The split part of the original cluster c will be removed, and filled in c1
bool PRadGEMCluster::splitCluster_sub(GEMPlaneCluster &c, GEMPlaneCluster &c1)
{
    // we use 2 consecutive iterator
    auto it = c.hits.begin();
    auto it_next = it + 1;

    // loop to find the local minimum
    bool descending = false, extremum = false;
    auto minimum = it;

    for(; it_next != c.hits.end(); ++it, ++it_next)
    {
        if(descending) {
            // update minimum
            if(it->charge < minimum->charge)
                minimum = it;

            // transcending trend, confirm a local minimum (valley)
            if(it_next->charge - it->charge > split_cluster_diff) {
                extremum = true;
                // only needs the first local minimum, thus exit the loop
                break;
            }
        } else {
            // descending trend, expect a local minimum
            if(it->charge - it_next->charge > split_cluster_diff) {
                descending = true;
                minimum = it_next;
            }
        }
    }

    if(extremum) {
        // half the charge of overlap strip
        minimum->charge /= 2.;

        // new split cluster
        c1 = GEMPlaneCluster(std::vector<GEMPlaneHit>(minimum, c.hits.end()));

        // remove the hits that are moved into new cluster, but keep the minimum
        c.hits.erase(std::next(minimum, 1), c.hits.end());
    }

    return extremum;
}

// filter out bad clusters
void PRadGEMCluster::filterCluster(std::list<GEMPlaneCluster> &cluster_list)
{
    for(auto it = cluster_list.begin(); it != cluster_list.end(); ++it)
    {
        // did not pass the filter, carefully remove element inside the loop
        if(!filterCluster_sub(*it))
            cluster_list.erase(it--);
    }
}

// this function helps filterCluster, it returns true if the cluster is good
// and return false if the cluster is bad
bool PRadGEMCluster::filterCluster_sub(const GEMPlaneCluster &c)
{
    // only check size for now
    if((c.hits.size() < min_cluster_hits) ||
       (c.hits.size() > max_cluster_hits))
        return false;

    // passed all the check
    return true;
}

// reconstruct cluster, calculate the cluster position
// it needs GEM plane as input to get the position of that strip on the plane
void PRadGEMCluster::reconstructCluster(std::list<GEMPlaneCluster> &cluster_list, PRadGEMPlane *plane)
{
    for(auto &cluster : cluster_list)
    {
        reconstructCluster_sub(cluster, plane);
    }
}

// this function helps reconstructCluster
// it reconstruct the position of cluster using linear weight of charge portion
void PRadGEMCluster::reconstructCluster_sub(GEMPlaneCluster &c, PRadGEMPlane *plane)
{
    // here determine position, peak charge and total charge of the cluster
    c.total_charge = 0.;
    c.peak_charge = 0.;
    float weight_pos = 0.;

    // no hits
    if(!c.hits.size())
        return;

    for(auto &hit : c.hits)
    {
        if(c.peak_charge < hit.charge)
            c.peak_charge = hit.charge;

        c.total_charge += hit.charge;

        weight_pos += plane->GetStripPosition(hit.strip)*hit.charge;
    }

    c.position = weight_pos/c.total_charge;
}

// this function accepts x, y clusters from detectors and then form GEM Cluster
// it return the number of clusters
void PRadGEMCluster::FormClusters(PRadGEMDetector *det)
{
    PRadGEMPlane *x_plane = det->GetPlane(PRadGEMPlane::Plane_X);
    PRadGEMPlane *y_plane = det->GetPlane(PRadGEMPlane::Plane_Y);

    // sanity check
    if(x_plane == nullptr || y_plane == nullptr) {
        std::cerr << "PRad GEM Cluster Error: Input plane is not instantiated, cannot "
                  << "form clusters"
                  << std::endl;
        return;
    }

    // get x, y plane clusters
    auto x_cluster = x_plane->GetPlaneClusters();
    auto y_cluster = y_plane->GetPlaneClusters();

    // get contianer and fill the hits
    auto &container = det->gem_clusters;

    // empty first
    container.clear();

    // TODO, probably add some criteria here to filter out some bad clusters
    // fill possible clusters in
    for(auto &xc : x_cluster)
    {
        for(auto &yc : y_cluster)
        {
            container.emplace_back(xc.position, yc.position, 0.,    // by default z = 0
                                   xc.total_charge, yc.total_charge,
                                   xc.peak_charge, yc.peak_charge,  // fill in peak charge
                                   xc.hits.size(), yc.hits.size()); // number of hits
        }
    }
}
