#ifndef PRAD_HYCAL_DETECTOR_H
#define PRAD_HYCAL_DETECTOR_H

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include "PRadException.h"
#include "PRadHyCalModule.h"
#include "PRadDetector.h"
#include "PRadEventStruct.h"

class PRadHyCalSystem;
class PRadHyCalCluster;
// these two structure will be used for cluster reconstruction, defined at the end
struct ModuleHit;
struct ModuleCluster;

class PRadHyCalDetector : public PRadDetector
{
public:
    friend class PRadHyCalSystem;

public:
    // constructor
    PRadHyCalDetector(const std::string &name = "HyCal", PRadHyCalSystem *sys = nullptr);

    // copy/move constructors
    PRadHyCalDetector(const PRadHyCalDetector &that);
    PRadHyCalDetector(PRadHyCalDetector &&that);

    // desctructor
    virtual ~PRadHyCalDetector();

    // copy/move assignment operators
    PRadHyCalDetector &operator =(const PRadHyCalDetector &rhs);
    PRadHyCalDetector &operator =(PRadHyCalDetector &&rhs);

    // public member functions
    void SetSystem(PRadHyCalSystem *sys, bool force_set = false);
    void UnsetSystem(bool force_unset =false);
    virtual void ReadModuleList(const std::string &path);
    void ReadCalibrationFile(const std::string &path);
    void SaveModuleList(const std::string &path) const;
    void SaveCalibrationFile(const std::string &path) const;
    bool AddModule(PRadHyCalModule *module);
    void RemoveModule(int id);
    void RemoveModule(const std::string &name);
    void RemoveModule(PRadHyCalModule *module);
    void DisconnectModule(int id, bool force_disconn = false);
    void DisconnectModule(const std::string &name, bool force_disconn = false);
    void DisconnectModule(PRadHyCalModule *module, bool force_disconn = false);
    void SortModuleList();
    void ClearModuleList();
    void OutputModuleList(std::ostream &os) const;
    void Reset();

    // hits/clusters reconstruction
    void Reconstruct(PRadHyCalCluster *method);
    void CollectHits();
    void ClearHits();

    // get parameters
    PRadHyCalSystem *GetSystem() const {return system;};
    PRadHyCalModule *GetModule(const int &primex_id) const;
    PRadHyCalModule *GetModule(const std::string &module_name) const;
    double GetEnergy() const;
    const std::vector<PRadHyCalModule*> &GetModuleList() const {return module_list;};
    const std::vector<ModuleHit> &GetModuleHits() const {return module_hits;};
    const std::vector<ModuleCluster> &GetModuleClusters() const {return module_clusters;};
    std::vector<HyCalHit> &GetHits() {return hycal_hits;};
    const std::vector<HyCalHit> &GetHits() const {return hycal_hits;};

protected:
    PRadHyCalSystem *system;
    std::vector<PRadHyCalModule*> module_list;
    std::unordered_map<int, PRadHyCalModule*> id_map;
    std::unordered_map<std::string, PRadHyCalModule*> name_map;
    std::vector<ModuleHit> module_hits;
    std::vector<ModuleCluster> module_clusters;
    std::vector<HyCalHit> hycal_hits;
};

struct ModuleHit
{
    int id;                         // module id
    PRadHyCalModule::Geometry geo;  // geometry
    float energy;                   // participated energy, may be splitted

    ModuleHit() : id(0), energy(0) {};
    ModuleHit(int i, const PRadHyCalModule::Geometry &g, float e)
    : id(i), geo(g), energy(e)
    {};
};

struct ModuleCluster
{
    ModuleHit center;
    std::vector<ModuleHit> hits;    // hits group
    float energy;

    ModuleCluster() : energy(0) {};
    ModuleCluster(const ModuleHit &hit) : center(hit), energy(0) {};

    void AddHit(const ModuleHit &hit)
    {
        hits.emplace_back(hit);
        energy += hit.energy;
    }

    void Merge(const ModuleCluster &that)
    {
        hits.reserve(hits.size() + that.hits.size());
        for(auto &hit : that.hits)
        {
            AddHit(hit);
        }
        if(center.energy < that.center.energy)
            center = that.center;
    }
};

#endif
