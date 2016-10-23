#ifndef PRAD_GEM_DETECTOR_H
#define PRAD_GEM_DETECTOR_H

#include <vector>
#include <string>
#include "PRadException.h"
#include "PRadGEMPlane.h"
#include "PRadEventStruct.h"
#include "PRadDetectors.h"

#define MAX_GCLUSTERS 250

class PRadGEMSystem;
class PRadGEMCluster;
class PRadGEMAPV;

class PRadGEMDetector
{
public:
    PRadGEMDetector(PRadGEMSystem *gem_srs,
                    const std::string &readoutBoard,
                    const std::string &detectorType,
                    const std::string &detector);
    virtual ~PRadGEMDetector();

    void AddPlane(const int &type, PRadGEMPlane *plane);
    void AddPlane(const PRadGEMPlane::PlaneType &type, const std::string &name,
                  const double &size, const int &conn, const int &ori, const int &dir);
    void ConnectAPV(const int &type, PRadGEMAPV *apv);
    void ReconstructHits(PRadGEMCluster *c);
    void ReconstructHits();
    void ClearHits();
    GEMHit *GetClusters(int &n);
    std::vector<GEMHit> GetClusters();

    // get parameters
    int GetID() {return id;};
    int GetDetID() {return det_id;};
    std::string GetName() {return name;};
    std::string GetType() {return type;};
    std::string GetReadoutBoard() {return readout_board;};
    PRadGEMPlane *GetPlane(const int &type);
    std::vector<PRadGEMPlane*> GetPlaneList();
    std::vector<PRadGEMAPV*> GetAPVList(const int &type);
    std::list<GEMPlaneCluster> &GetPlaneClusters(const int &type) throw(PRadException);
    std::vector<std::list<GEMPlaneCluster>*> GetDetectorClusters();

    // set parameters
    void AssignID(const int &i);

private:
    PRadGEMSystem *gem_srs;
    int id;
    int det_id;
    std::string name;
    std::string type;
    std::string readout_board;
    std::vector<PRadGEMPlane*> planes;
    GEMHit gem_clusters[MAX_GCLUSTERS];
    int NClusters;
};

#endif
