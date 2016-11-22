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

class PRadHyCalDetector : public PRadDetector
{
public:
    friend class PRadHyCalCluster;
    friend class PRadSquareCluster;
    friend class PRadIslandCluster;

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

    // get parameters
    PRadHyCalSystem *GetSystem() const {return system;};
    PRadHyCalModule *GetModule(const int &primex_id) const;
    PRadHyCalModule *GetModule(const std::string &module_name) const;
    double GetEnergy() const;
    const std::vector<PRadHyCalModule*> &GetModuleList() const {return module_list;};
    std::vector<HyCalCluster> &GetCluster() {return hycal_clusters;};
    const std::vector<HyCalCluster> &GetCluster() const {return hycal_clusters;};

protected:
    PRadHyCalSystem *system;
    std::vector<PRadHyCalModule*> module_list;
    std::unordered_map<int, PRadHyCalModule*> id_map;
    std::unordered_map<std::string, PRadHyCalModule*> name_map;
    std::vector<HyCalCluster> hycal_clusters;
};

#endif
