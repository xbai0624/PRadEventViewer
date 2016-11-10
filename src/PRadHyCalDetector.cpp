//============================================================================//
// HyCal detector class                                                       //
//                                                                            //
// Chao Peng                                                                  //
// 11/11/2016                                                                 //
//============================================================================//

#include "PRadHyCalDetector.h"
#include "PRadHyCalSystem.h"
#include "PRadHyCalCluster.h"
#include <algorithm>
#include <iostream>



//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//

// constructor
PRadHyCalDetector::PRadHyCalDetector(const std::string &det, PRadHyCalSystem *sys)
: PRadDetector(detector), system(sys)
{
    // place holder
}

// copy and move assignment will copy or move the modules, but the connection to
// HyCal system and the connections between modules and DAQ units won't be copied
// copy constructor
PRadHyCalDetector::PRadHyCalDetector(const PRadHyCalDetector &that)
: PRadDetector(that), system(nullptr)
{
    for(auto module : that.module_list)
    {
        AddModule(PRadHyCalModule(*module));
    }
}

// move constructor
PRadHyCalDetector::PRadHyCalDetector(PRadHyCalDetector &&that)
: PRadDetector(that), system(nullptr), module_list(std::move(that.module_list)),
  id_map(std::move(that.id_map)), name_map(std::move(that.name_map))
{
    // reset the connections between module and HyCal
    for(auto module : module_list)
        module->SetDetector(this);
}

// destructor
PRadHyCalDetector::~PRadHyCalDetector()
{
    UnsetSystem();

    // release modules
    for(auto module : module_list)
    {
        // prevent module calling RemoveModule upon destruction
        module->UnsetDetector(true);
        delete module;
    }
}

// copy assignment operator
PRadHyCalDetector &PRadHyCalDetector::operator =(const PRadHyCalDetector &rhs)
{
    PRadHyCalDetector that(rhs); // use copy constructor
    *this = std::move(that);     // use move assignment
    return *this;
}

// move assignment operator
PRadHyCalDetector &PRadHyCalDetector::operator =(PRadHyCalDetector &&rhs)
{
    // disconnect HyCal system
    UnsetSystem();
    PRadDetector::operator=(rhs);
    module_list = std::move(rhs.module_list);
    id_map = std::move(rhs.id_map);
    name_map = std::move(rhs.name_map);

    for(auto module : module_list)
        module->SetDetector(this);

    return *this;
}

//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

// set a new system, disconnect from the previous one
void PRadHyCalDetector::SetSystem(PRadHyCalSystem *sys)
{
    UnsetSystem();
    system = sys;
}

void PRadHyCalDetector::UnsetSystem(bool system_destroy)
{
    if(system == nullptr)
        return;

    // if system is going to be destroyed, not need to call remove detector from it
    if(!system_destroy)
        system->RemoveDetector();

    system = nullptr;
}

// add a HyCal module to the detector
bool PRadHyCalDetector::AddModule(PRadHyCalModule *module)
{
    if(module == nullptr)
        return false;

    int id = module->GetID();

    if(id_map.find(id) != id_map.end()) {
        std::cerr << "PRad HyCal Detector Error: "
                  << "Module " << id << " exists, abort adding module "
                  << "with the same id."
                  << std::endl;
        return false;
    }

    const std::string &name = module->GetName();

    if(name_map.find(name) != name_map.end()) {
        std::cerr << "PRad HyCal Detector Error: "
                  << "Module " << name << " exists, abort adding module "
                  << "with the same name."
                  << std::endl;
        return false;
    }

    module_list.push_back(module);
    name_map[name] = module;
    id_map[id] = module;

    return true;
}

// remove module
void PRadHyCalDetector::RemoveModule(PRadHyCalModule *module)
{
    if(module == nullptr)
        return;

    module_list.erase(module);
    id_map.erase(module->GetID());
    name_map.erase(module->GetName());
}

void PRadHyCalDetector::SortModuleList()
{
    std::sort(module_list.begin(), module_list.end(),
             [](const PRadHyCalModule *m1, const PRadHyCalModule *m2)
             {
                return m1->GetID() < m2->GetID();
             });
}

void PRadHyCalDetector::ClearModuleList()
{
    for(auto module : module_list)
    {
        // prevent module calling RemoveModule upon destruction
        module->UnsetDetector(true);
        delete module;
    }

    module_list.clear();
    id_map.clear();
    name_map.clear();
}

PRadHyCalModule *PRadHyCalDetector::GetModule(const int &id)
const
{

}

PRadHyCalModule *PRadHyCalDetector::GetModule(const std::string &name)
const
{

}
