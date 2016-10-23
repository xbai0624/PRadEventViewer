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
PRadDetMatch::PRadDetMatch()
{
    leadGlassRes = 10.0;
    transitionRes = 7.0;
    crystalRes = 3.0;
    matchSigma = 5.0;
}

// destructor
PRadDetMatch::~PRadDetMatch()
{
    // place holder
}

// project 1 HyCal cluster and 1 GEM cluster to HyCal Plane.
// if they are at the same z, there will be no projection, otherwise they are
// projected to the furthest z
// return true if they are within certain range (depends on HyCal resolution)
// return false if they are not
bool PRadDetMatch::PreMatch(const HyCalHit &hycal, const GEMHit &gem)
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

/*
void PRadDetMatch::MatchProcessing()
{
    if(fHyCalGEMMatchMode) {
        //we have chosen the GEM hit that is closest to HyCal Hit in this mode
        //we will just do additional selection for hits appear in the overlap region
        for(int i=0; i<fNHyCalClusters; i++)
        {
            //if the HyCal Cluster has hits from both GEM

            if (fHyCalClusters[i].gemNClusters[0] == 1 && fHyCalClusters[i].gemNClusters[1] == 1){
                //project every thing to the second GEM for analysis
                int   index[NGEM] = { fHyCalClusters[i].gemClusterID[0][0], fHyCalClusters[i].gemClusterID[1][0]};
                float hycalX = fHyCalClusters[i].x_log;
                float hycalY = fHyCalClusters[i].y_log;
                float GEM1X  =  fGEM2DClusters[0].at(index[0]).x;
                float GEM1Y  =  fGEM2DClusters[0].at(index[0]).y;
                float GEM2X  =  fGEM2DClusters[1].at(index[1]).x;
                float GEM2Y  =  fGEM2DClusters[1].at(index[1]).y;

                ProjectToZ(hycalX, hycalY, fHyCalZ, fGEMZ[1]);
                ProjectToZ(GEM1X, GEM1Y, fGEMZ[0], fGEMZ[1]);

                float r1 = Distance2Points(hycalX, hycalY, GEM1X, GEM1Y);
                float r2 = Distance2Points(hycalX, hycalY, GEM2X, GEM2Y);
                float r3 = Distance2Points(GEM1X, GEM1Y, GEM2X, GEM2Y);
                if (r3 < 10.*fGEMResolution) {
                    //likely the two this from the two GEMs are from the same track
                    fHyCalClusters[i].x_gem = (GEM1X + GEM2X)/2.;
                    fHyCalClusters[i].y_gem = (GEM1Y + GEM2Y)/2.;
                    fHyCalClusters[i].z_gem = fGEMZ[1];
                    SET_BIT(fHyCalClusters[i].flag, kOverlapMatch);
                } else {
                    //will keep only one of the two hits
                    int isave = r1 < r2 ? 0 : 1;
                    int ikill = (int)(!isave);
                    fHyCalClusters[i].gemNClusters[ikill] = 0;
                    fHyCalClusters[i].x_gem = fGEM2DClusters[isave].at(index[isave]).x;
                    fHyCalClusters[i].y_gem = fGEM2DClusters[isave].at(index[isave]).y;
                    fHyCalClusters[i].z_gem = fGEMZ[isave];
                    if (ikill == 0) { CLEAR_BIT(fHyCalClusters[i].flag, kGEM1Match); }
                    else { CLEAR_BIT(fHyCalClusters[i].flag, kGEM2Match); }
                }

            }
             if (fHyCalClusters[i].gemNClusters[0] > 0 || fHyCalClusters[i].gemNClusters[1] >0 )
             SET_BIT(fHyCalClusters[i].flag, kGEMMatch);
        }
    } else {
        //TODO for more advanced GEM and HyCal match processing method
    }
}
*/
