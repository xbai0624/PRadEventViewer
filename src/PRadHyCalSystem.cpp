//============================================================================//
// PRad HyCal System                                                          //
//                                                                            //
// Chao Peng                                                                  //
// 11/12/2016                                                                 //
//============================================================================//

#include "PRadHyCalSystem.h"
#include <iostream>
#include <iomanip>



//============================================================================//
// Constructor, Destructor, Assignment Operators                              //
//============================================================================//

// constructor
PRadHyCalSystem::PRadHyCalSystem(const std::string &path)
: hycal(new PRadHyCalDetector("HyCal", this))
{
    Configure(path);
}

// destructor
PRadHyCalSystem::~PRadHyCalSystem()
{
    delete hycal;
}



//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

// configure HyCal System
void PRadHyCalSystem::Configure(const std::string &path)
{
    if(path.empty())
        return;

    readConfigFile(path);

    ReadModuleList(GetConfig<std::string>("Module List"));
}

// read module list
void PRadHyCalSystem::ReadModuleList(const std::string &path)
{
    if(hycal == nullptr) {
        std::cerr << "PRad HyCal System Error: Has no detector, cannot read "
                  << "module list."
                  << std::endl;
        return;
    }

    ConfigParser c_parser;
    if(!c_parser.ReadFile(path)) {
        std::cerr << "PRad HyCal System Error: Failed to read module list file "
                  << "\"" << path << "\"."
                  << std::endl;
        return;
    }

    std::string name;
    std::string type, sector;
    PRadHyCalModule::Geometry geo;

    // some info that is not read from list
    while (c_parser.ParseLine())
    {
        if(!c_parser.CheckElements(9))
            continue;

        c_parser >> name >> type
                 >> geo.size_x >> geo.size_y >> geo.x >> geo.y
                 >> sector
                 >> geo.row >> geo.column;

        geo.type = PRadHyCalModule::get_module_type(type.c_str());
        geo.sector = PRadHyCalModule::get_sector_id(sector.c_str());

        PRadHyCalModule *module = new PRadHyCalModule(name, geo);

        // failed to add module to detector
        if(!hycal->AddModule(module))
            delete module;
    }
}

// add detector, remove the original detector
void PRadHyCalSystem::AddDetector(PRadHyCalDetector *h)
{
    if(hycal)
        hycal->UnsetSystem();

    hycal = h;
    hycal->SetSystem(this);
}
// remove current detector
void PRadHyCalSystem::RemoveDetector()
{
    hycal = nullptr;
}

