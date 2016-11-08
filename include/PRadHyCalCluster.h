#ifndef PRAD_HYCAL_CLUSTER_H
#define PRAD_HYCAL_CLUSTER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "PRadDataHandler.h"
#include "PRadDAQUnit.h"
#include "PRadTDCGroup.h"
#include "PRadEventStruct.h"
#include "PRadDetector.h"
#include "ConfigObject.h"

#define MAX_HCLUSTERS 250 // Maximum storage

class PRadHyCalCluster : public ConfigObject
{
public:
    PRadHyCalCluster(PRadDataHandler *h = nullptr);
    virtual ~PRadHyCalCluster();

    void SetHandler(PRadDataHandler *h);

    // functions that to be overloaded
    virtual void Reconstruct(EventData &event);
    virtual void Clear();
    virtual int GetNClusters() {return fNHyCalClusters;};
    virtual HyCalHit *GetCluster() {return fHyCalCluster;};


protected:
    PRadDataHandler *fHandler;
    // result array
    int fNHyCalClusters;
    HyCalHit fHyCalCluster[MAX_HCLUSTERS];
};

#endif
