#ifndef PRAD_GEM_DETECTOR_H
#define PRAD_GEM_DETECTOR_H

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include "PRadException.h"
#include "PRadHyCalModule.h"
#include "PRadDetector.h"

class PRadHyCalSystem;

class PRadHyCalDetector : public PRadDetector
{
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
    bool AddModule(PRadHyCalModule *module);
    void RemoveModule(PRadHyCalModule *module);
    void SortModuleList();
    void ClearModuleList();
    void OutputModuleList(std::ostream &os);
    void ConnectDAQ();
    void ReconstructHits();
    void ClearHits();

    // get parameters
    PRadHyCalSystem *GetSystem() const {return system;};
    PRadHyCalModule *GetModule(const int &primex_id) const;
    PRadHyCalModule *GetModule(const std::string &module_name) const;
    std::vector<PRadHyCalModule*> GetModuleList() const {return module_list;};

private:
    PRadHyCalSystem *system;
    std::vector<PRadHyCalModule*> module_list;
    std::unordered_map<int, PRadHyCalModule*> id_map;
    std::unordered_map<std::string, PRadHyCalModule*> name_map;
};

#endif
