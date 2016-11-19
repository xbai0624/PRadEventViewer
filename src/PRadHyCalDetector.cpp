//============================================================================//
// HyCal detector class                                                       //
//                                                                            //
// Chao Peng                                                                  //
// 11/11/2016                                                                 //
//============================================================================//

#include "PRadHyCalDetector.h"
#include "PRadHyCalSystem.h"
#include "TH1.h"
#include <algorithm>
#include <fstream>
#include <iomanip>



//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//

// constructor
PRadHyCalDetector::PRadHyCalDetector(const std::string &det, PRadHyCalSystem *sys)
: PRadDetector(det), system(sys)
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
        AddModule(new PRadHyCalModule(*module));
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
    PRadDetector::operator =(rhs);
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
void PRadHyCalDetector::SetSystem(PRadHyCalSystem *sys, bool force_set)
{
    if(sys == system)
        return;

    // force to set system without unset original system
    // it will be useful for destruction
    if(!force_set)
        UnsetSystem();

    system = sys;
}

void PRadHyCalDetector::UnsetSystem(bool force_unset)
{
    // only do this if system exists
    if(!system)
        return;

    if(!force_unset)
        system->DisconnectDetector();

    system = nullptr;
}

// read module list
void PRadHyCalDetector::ReadModuleList(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    if(!c_parser.ReadFile(path)) {
        std::cerr << "PRad HyCal Detector Error: Failed to read module list file "
                  << "\"" << path << "\"."
                  << std::endl;
        return;
    }

    // clear all modules
    ClearModuleList();

    std::string name;
    std::string type, sector;
    PRadHyCalModule::Geometry geo;

    // some info that is not read from list
    while (c_parser.ParseLine())
    {
        if(!c_parser.CheckElements(11))
            continue;

        c_parser >> name >> type
                 >> geo.size_x >> geo.size_y >> geo.size_z
                 >> geo.x >> geo.y >> geo.z
                 >> sector >> geo.row >> geo.column;

        geo.type = PRadHyCalModule::get_module_type(type.c_str());
        geo.sector = PRadHyCalModule::get_sector_id(sector.c_str());

        PRadHyCalModule *module = new PRadHyCalModule(name, geo);

        // failed to add module to detector
        if(!AddModule(module))
            delete module;
    }

    // sort the module by id
    SortModuleList();
}

// read calibration constants file
void PRadHyCalDetector::ReadCalibrationFile(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    if(!c_parser.ReadFile(path)) {
        std::cerr << "PRad HyCal Detector Error: Failed to read calibration file "
                  << " \"" << path << "\""
                  << std::endl;
        return;
    }

    std::string name;
    double factor, Ecal, nl;

    while(c_parser.ParseLine())
    {
        if(!c_parser.CheckElements(4, -1)) // more than 4 elements
            continue;

        c_parser >> name >> factor >> Ecal >> nl;
        std::vector<double> gains;
        while(c_parser.NbofElements())
        {
            gains.push_back(c_parser.TakeFirst<double>());
        }

        PRadCalibConst cal_const(factor, Ecal, nl, gains);

        PRadHyCalModule *module = GetModule(name);
        if(module) {
            module->SetCalibConst(cal_const);
        } else {
            std::cout << "PRad HyCal Detector Warning: Cannot find HyCal module "
                      << name << ", skipped its update for calibration constant."
                      << std::endl;
        }
    }
}

void PRadHyCalDetector::SaveModuleList(const std::string &path)
const
{
    std::ofstream outf(path);

    OutputModuleList(outf);

    outf.close();
}

void PRadHyCalDetector::SaveCalibrationFile(const std::string &path)
const
{
    std::ofstream outf(path);

    for(auto &module : module_list)
    {
        outf << std::setw(8) << module->GetName()
             << module->GetCalibConst()
             << std::endl;
    }

    outf.close();
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

    module->SetDetector(this);
    module_list.push_back(module);
    name_map[name] = module;
    id_map[id] = module;

    return true;
}

// remove module
void PRadHyCalDetector::RemoveModule(int id)
{
    RemoveModule(GetModule(id));
}

void PRadHyCalDetector::RemoveModule(const std::string &name)
{
    RemoveModule(GetModule(name));
}

void PRadHyCalDetector::RemoveModule(PRadHyCalModule *module)
{
    if(!module)
        return;

    id_map.erase(module->GetID());
    name_map.erase(module->GetName());

    module->UnsetDetector(true);
    delete module;

    // rebuild module list
    module_list.clear();
    for(auto &it : id_map)
        module_list.push_back(it.second);
}

// disconnect module
void PRadHyCalDetector::DisconnectModule(int id, bool force_disconn)
{
    DisconnectModule(GetModule(id), force_disconn);
}

void PRadHyCalDetector::DisconnectModule(const std::string &name, bool force_disconn)
{
    DisconnectModule(GetModule(name), force_disconn);
}

void PRadHyCalDetector::DisconnectModule(PRadHyCalModule *module, bool force_disconn)
{
    if(!module)
        return;

    id_map.erase(module->GetID());
    name_map.erase(module->GetName());

    if(!force_disconn)
        module->UnsetDetector(true);

    // rebuild module list
    module_list.clear();
    for(auto &it : id_map)
        module_list.push_back(it.second);
}

void PRadHyCalDetector::SortModuleList()
{
    std::sort(module_list.begin(), module_list.end(),
             [](const PRadHyCalModule *m1, const PRadHyCalModule *m2)
             {
                return *m1 < *m2;
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

void PRadHyCalDetector::OutputModuleList(std::ostream &os)
const
{
    for(auto module : module_list)
    {
        os << *module << std::endl;
    }
}

PRadHyCalModule *PRadHyCalDetector::GetModule(const int &id)
const
{
    auto it = id_map.find(id);
    if(it == id_map.end())
        return nullptr;
    return it->second;
}

PRadHyCalModule *PRadHyCalDetector::GetModule(const std::string &name)
const
{
    auto it = name_map.find(name);
    if(it == name_map.end())
        return nullptr;
    return it->second;
}

double PRadHyCalDetector::GetEnergy()
const
{
    double energy = 0.;
    for(auto &module : module_list)
    {
        energy += module->GetEnergy();
    }
    return energy;
}


