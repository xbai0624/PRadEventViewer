//========================================================//
//match the HyCal Clusters and GEM clusters, both of them //
//need to be transformed to the Lab frame in advance      //
//========================================================//

#ifndef PRAD_DET_MATCH_H
#define PRAD_DET_MATCH_H

#include "PRadEventStruct.h"

class PRadDetMatch
{
public:
    PRadDetMatch();
    virtual ~PRadDetMatch();

    bool PreMatch(const HyCalHit &h, const GEMHit &g);

private:
    float leadGlassRes;
    float crystalRes;
    float transitionRes;
    float matchSigma;
};

#endif
