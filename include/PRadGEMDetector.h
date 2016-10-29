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
    // constructor
    PRadGEMDetector(const std::string &readoutBoard,
                    const std::string &detectorType,
                    const std::string &detector,
                    PRadGEMSystem *g = nullptr);

    // copy/move constructors
    PRadGEMDetector(const PRadGEMDetector &that);
    PRadGEMDetector(PRadGEMDetector &&that);

    // desctructor
    virtual ~PRadGEMDetector();

    // copy/move assignment operators
    PRadGEMDetector &operator =(const PRadGEMDetector &rhs);
    PRadGEMDetector &operator =(PRadGEMDetector &&rhs);

    // public member functions
    void SetSystem(PRadGEMSystem *sys);
    void UnsetSystem(bool system_destroy = false);
    bool AddPlane(PRadGEMPlane *plane);
    bool AddPlane(const int &type, const std::string &name, const double &size,
                  const int &conn, const int &ori, const int &dir);
    void RemovePlane(const int &type);
    void ConnectPlanes();
    void ReconstructHits(PRadGEMCluster *c);
    void ReconstructHits();
    void ClearHits();

    // get parameters
    int GetID() const {return id;};
    int GetDetID() const {return det_id;};
    const std::string &GetName() const {return name;};
    const std::string &GetType() const {return type;};
    const std::string &GetReadoutBoard() {return readout_board;};
    PRadGEMPlane *GetPlane(const int &type) const;
    PRadGEMPlane *GetPlane(const std::string &type) const;
    std::vector<PRadGEMPlane*> GetPlaneList() const;
    std::vector<PRadGEMAPV*> GetAPVList(const int &type) const;
    int GetNClusters() const {return (int)gem_clusters.size();};
    GEMHit *GetCluster(int &n);
    std::vector<GEMHit> &GetCluster() {return gem_clusters;};
    const std::vector<GEMHit> &GetCluster() const {return gem_clusters;};

private:
    PRadGEMSystem *gem_srs;
    int id;
    int det_id;
    std::string name;
    std::string type;
    std::string readout_board;
    std::vector<PRadGEMPlane*> planes;
    std::vector<GEMHit> gem_clusters;
};

#endif
