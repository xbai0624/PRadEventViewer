#ifndef PRAD_GEM_PLANE_H
#define PRAD_GEM_PLANE_H

#include <list>
#include <vector>
#include <string>

// unit is mm
// X plane did a shift to move the X origin to center hole
// origin shift: 550.4/2 - (44-pitch)/2 = 253.2 mm
#define STRIP_PITCH 0.4
#define X_SHIFT 253.2


class PRadGEMDetector;
class PRadGEMAPV;
// these two structure will be used as data type
// defined at the end
struct GEMPlaneHit;
struct GEMPlaneCluster;

class PRadGEMPlane
{
public:
    enum PlaneType
    {
        Plane_X,
        Plane_Y,
        Plane_Max
    };

    static const char *GetPlaneTypeName(int enumVal);
    static int GetPlaneTypeID(const char *name);

public:
    // constructors
    PRadGEMPlane(PRadGEMDetector *det = nullptr);
    PRadGEMPlane(const std::string &n, const int &t, const float &s, const int &c,
                 const int &o, const int &d, PRadGEMDetector *det = nullptr);

    // copy/move constructors
    PRadGEMPlane(const PRadGEMPlane &that);
    PRadGEMPlane(PRadGEMPlane &&that);

    // destructor
    virtual ~PRadGEMPlane();

    // copy/move assignment operators
    PRadGEMPlane &operator= (const PRadGEMPlane &rhs);
    PRadGEMPlane &operator= (PRadGEMPlane &&rhs);

    // public member functions
    void ConnectAPV(PRadGEMAPV *apv, const int &index);
    void DisconnectAPV(const size_t &plane_index);
    void ResetConnections();
    void AddPlaneHit(const int &plane_strip, const std::vector<float> &charges);
    void ClearPlaneHits();
    void CollectAPVHits();
    float GetStripPosition(const int &plane_strip) const;
    float GetMaxCharge(const std::vector<float> &charges) const;
    float GetIntegratedCharge(const std::vector<float> &charges) const;

    // set parameter
    void SetDetector(PRadGEMDetector *det);
    void UnsetDetector();
    void SetName(const std::string &n) {name = n;};
    void SetType(const PlaneType &t) {type = t;};
    void SetSize(const float &s) {size = s;};
    void SetOrientation(const int &o) {orient = o;};
    void SetCapacity(int c);

    // get parameter
    PRadGEMDetector *GetDetector() const {return detector;};
    const std::string &GetName() const {return name;};
    PlaneType GetType() const {return type;};
    float GetSize() const {return size;};
    int GetCapacity() const {return apv_list.size();};
    int GetOrientation() const {return orient;};
    std::vector<PRadGEMAPV*> GetAPVList() const;
    std::vector<GEMPlaneHit> &GetPlaneHits() {return hit_list;};
    const std::vector<GEMPlaneHit> &GetPlaneHits() const {return hit_list;};
    std::list<GEMPlaneCluster> &GetPlaneClusters() {return cluster_list;};
    const std::list<GEMPlaneCluster> &GetPlaneClusters() const {return cluster_list;};

private:
    PRadGEMDetector *detector;
    std::string name;
    PlaneType type;
    float size;
    int connector;
    int orient;
    int direction;
    std::vector<PRadGEMAPV*> apv_list;

    // plane data
    std::vector<GEMPlaneHit> hit_list;
    // there will be requent remove, split operations for clusters in the middle
    // thus use list instead of vector
    std::list<GEMPlaneCluster> cluster_list;
};

struct GEMPlaneHit
{
    int strip;
    float charge;

    GEMPlaneHit() : strip(0), charge(0.) {};
    GEMPlaneHit(const int &s, const float &c)
    : strip(s), charge(c) {};
};

struct GEMPlaneCluster
{
    float position;
    float peak_charge;
    float total_charge;
    std::vector<GEMPlaneHit> hits;

    GEMPlaneCluster()
    : position(0.), peak_charge(0.), total_charge(0.)
    {};

    GEMPlaneCluster(const std::vector<GEMPlaneHit> &p)
    : position(0.), peak_charge(0.), total_charge(0.), hits(p)
    {};

    GEMPlaneCluster(std::vector<GEMPlaneHit> &&p)
    : position(0.), peak_charge(0.), total_charge(0.), hits(std::move(p))
    {};
};

#endif
