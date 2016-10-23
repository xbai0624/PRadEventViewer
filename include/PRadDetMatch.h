//========================================================//
//match the HyCal Clusters and GEM clusters, both of them //
//need to be transformed to the Lab frame in advance      //
//========================================================//

#ifndef PRAD_DET_MATCH_H
#define PRAD_DET_MATCH_H

#include <vector>
#include "PRadEventStruct.h"

struct MatchedIndex
{
    int hycal;
    int gem1;
    int gem2;
    std::vector<int> gem1_cand;
    std::vector<int> gem2_cand;

    MatchedIndex()
    : hycal(-1), gem1(-1), gem2(-1)
    {};
    MatchedIndex(int idx)
    : hycal(idx), gem1(-1), gem2(-1)
    {};
};

class PRadDetMatch
{
public:
    PRadDetMatch();
    virtual ~PRadDetMatch();

    std::vector<MatchedIndex> Match(HyCalHit *hycal, int nHyCal,
                                    GEMHit *gem1, int nGEM1,
                                    GEMHit *gem2, int nGEM2) const;
    bool PreMatch(const HyCalHit &h, const GEMHit &g) const;
    bool PostMatch(MatchedIndex &idx, HyCalHit &h, GEMHit *g1, GEMHit *g2) const;

private:
    float gemRes;
    float leadGlassRes;
    float crystalRes;
    float transitionRes;
    float matchSigma;
    float overlapSigma;
};

#endif
