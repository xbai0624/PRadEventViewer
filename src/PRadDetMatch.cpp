//============================================================================//
// Match the HyCal Clusters and GEM clusters                                  //
// The clusters are required to be in the same frame (beam center frame)      //
// The frame transform work can be done in PRadCoordSystem                    //
//                                                                            //
// Details on projection:                                                     //
// By calling PRadCoordSystem::ProjectionDistance, the z from two clusters    //
// are compared first, if they are not at the same XOY-plane, the one with    //
// smaller z will be projected to the larger z. (from target point (0, 0, 0)) //
// If do not want the automatically projection, be sure you have projected    //
// the clusters to the same XOY-plane first.                                  //
//                                                                            //
//                                                                            //
// Xinzhan Bai, first version                                                 //
// Weizhi Xiong, adapted to PRad analysis software                            //
// Chao Peng, removed coordinates manipulation, only left matching part       //
// 10/22/2016                                                                 //
//============================================================================//

#include "PRadDetMatch.h"
#include "PRadCoordSystem.h"

// constructor
PRadDetMatch::PRadDetMatch(const std::string &path)
{
    Configure(path);
    leadGlassRes = 10.0;
    transitionRes = 7.0;
    crystalRes = 3.0;
    gemRes = 0.08;
    matchSigma = 5.0;
    overlapSigma = 10.0;
}

// destructor
PRadDetMatch::~PRadDetMatch()
{
    // place holder
}

void PRadDetMatch::Configure(const std::string &path)
{
    // if no configuration file specified, load the default value quietly
    bool verbose = false;

    if(!path.empty()) {
        readConfigFile(path);
        verbose = true;
    }

    leadGlassRes = getConfigValue("Lead_Glass_Resolution", "10", verbose).Float();
    transitionRes = getConfigValue("Transition_Resolution", "7", verbose).Float();
    crystalRes = getConfigValue("Crystal_Resolution", "3", verbose).Float();
    gemRes = getConfigValue("GEM_Resolution", "0.08", verbose).Float();
    matchSigma = getConfigValue("Match_Factor", "5", verbose).Float();
    overlapSigma = getConfigValue("GEM_Overlap_Factor", "10", verbose).Float();

}

std::vector<MatchedIndex> PRadDetMatch::Match(HyCalHit *hycal, int nHyCal,
                                              GEMHit *gem1, int nGEM1,
                                              GEMHit *gem2, int nGEM2)
const
{
    std::vector<MatchedIndex> result;

    for(int i = 0; i < nHyCal; ++i)
    {
        MatchedIndex index(i);

        // pre match, only check if distance is within the range
        // fill in hits as candidates
        for(int j = 0; j < nGEM1; ++j)
        {
            if(PreMatch(hycal[i], gem1[j]))
                index.gem1_cand.push_back(j);
        }
        for(int j = 0; j < nGEM2; ++j)
        {
            if(PreMatch(hycal[i], gem2[j]))
                index.gem2_cand.push_back(j);
        }

        // post match, do further check, which one is the closest
        // and if the two gem hits are overlapped
        if(PostMatch(index, hycal[i], gem1, gem2))
            result.push_back(index);
    }

    return result;
}

// project 1 HyCal cluster and 1 GEM cluster to HyCal Plane.
// if they are at the same z, there will be no projection, otherwise they are
// projected to the furthest z
// return true if they are within certain range (depends on HyCal resolution)
// return false if they are not
bool PRadDetMatch::PreMatch(const HyCalHit &hycal, const GEMHit &gem)
const
{
    // lead glass (largest) value as default
    float base_range = leadGlassRes;
    // crystal region
    if(TEST_BIT(hycal.flag, kPWO))
        base_range = crystalRes;
    // transition region
    if(TEST_BIT(hycal.flag, kTransition))
        base_range = transitionRes;

    float dist = PRadCoordSystem::ProjectionDistance(hycal, gem);

    if(dist > matchSigma * base_range)
        return false;
    else
        return true;
}

bool PRadDetMatch::PostMatch(MatchedIndex &index, HyCalHit &hycal, GEMHit *gem1, GEMHit *gem2)
const
{
    // no candidates
    if(index.gem1_cand.empty() && index.gem2_cand.empty())
        return false;

    // find the closest hits from gem1 and gem2
    // large initial value
    float dist = 1e3;
    for(auto &idx : index.gem1_cand)
    {
        float new_dist = PRadCoordSystem::ProjectionDistance(hycal, gem1[idx]);
        if(new_dist < dist) {
            dist = new_dist;
            index.gem1 = idx;
        }
    }

    dist = 1e3;
    for(auto &idx : index.gem2_cand)
    {
        float new_dist = PRadCoordSystem::ProjectionDistance(hycal, gem2[idx]);
        if(new_dist < dist) {
            dist = new_dist;
            index.gem2 = idx;
        }
    }

    // no gem1 hits matched
    if(index.gem1 == -1) {
        SET_BIT(hycal.flag, kGEM2Match);
    // no gem2 hits matched
    } else if(index.gem2 == -1) {
        SET_BIT(hycal.flag, kGEM1Match);
    // both gem1 and gem2 have matched hits, check if they are overlapped
    } else {
        float gem_dist = PRadCoordSystem::ProjectionDistance(gem1[index.gem1], gem2[index.gem2]);
        if(gem_dist < overlapSigma * gemRes) {
            SET_BIT(hycal.flag, kOverlapMatch);
        } else {
            float gem1_dist = PRadCoordSystem::ProjectionDistance(gem1[index.gem1], hycal);
            float gem2_dist = PRadCoordSystem::ProjectionDistance(gem2[index.gem2], hycal);
            if(gem1_dist < gem2_dist) {
                index.gem2 = -1;
                SET_BIT(hycal.flag, kGEM1Match);
            } else {
                index.gem1 = -1;
                SET_BIT(hycal.flag, kGEM2Match);
            }
        }
    }

    return true;
}
