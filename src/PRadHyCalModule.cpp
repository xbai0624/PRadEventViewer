//============================================================================//
// Basic DAQ channel unit                                                     //
// It could be HyCal Module, LMS PMT or Scintillator                          //
// Chao Peng                                                                  //
// 02/17/2016                                                                 //
//============================================================================//

#include "PRadHyCalModule.h"
#include "PRadHyCalDetector.h"
#include <exception>
#include <iomanip>
#include <cstring>


// enum name lists
static const char *__module_type_list[] = {"PbGlass", "PbWO4"};
static const char *__hycal_sector_list[] = {"Center", "Top", "Right", "Bottom", "Left"};



//============================================================================//
// Constructors, Destructor, Assignment Operators                             //
//============================================================================//

// constructors
PRadHyCalModule::PRadHyCalModule(const std::string &n,
                                 const Geometry &geo,
                                 PRadHyCalDetector *det)
: detector(det), name(n), geometry(geo)
{
    id = name_to_primex_id(n);
}

PRadHyCalModule::PRadHyCalModule(int pid, const Geometry &geo, PRadHyCalDetector *det)
: detector(det), id(pid), geometry(geo)
{
    if(geo.type == PbGlass)
        name = "G";
    if(geo.type == PbWO4)
        name = "W";

    name += std::to_string(id);
}

PRadHyCalModule::PRadHyCalModule(const std::string &n,
                                 int type, double size_x, double size_y, double x, double y,
                                 PRadHyCalDetector *det)
: detector(det), name(n)
{
    id = name_to_primex_id(name);
    geometry = Geometry(type, size_x, size_y, x, y);
    get_sector_info(id, geometry.sector, geometry.row, geometry.column);
}
// copy constructor
PRadHyCalModule::PRadHyCalModule(const PRadHyCalModule &that)
: detector(nullptr), name(that.name), id(that.id), geometry(that.geometry)
{
    // place holder
}

// move constructor
PRadHyCalModule::PRadHyCalModule(PRadHyCalModule &&that)
: detector(nullptr), name(std::move(that.name)), id(that.id), geometry(that.geometry)
{
    // place holder
}

// destructor
PRadHyCalModule::~PRadHyCalModule()
{
    UnsetDetector();
}

// copy assignment operator
PRadHyCalModule &PRadHyCalModule::operator =(const PRadHyCalModule &rhs)
{
    name = rhs.name;
    id = rhs.id;
    geometry = rhs.geometry;
    return *this;
}

PRadHyCalModule &PRadHyCalModule::operator =(PRadHyCalModule &&rhs)
{
    name = std::move(rhs.name);
    id = rhs.id;
    geometry = rhs.geometry;
    return *this;
}

//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

// set detector to the plane
void PRadHyCalModule::SetDetector(PRadHyCalDetector *det, bool force_set)
{
    if(det == detector)
        return;

    if(!force_set)
        UnsetDetector();

    detector = det;
}

// disconnect the detector
void PRadHyCalModule::UnsetDetector(bool force_unset)
{
    if(!detector)
        return;

    if(!force_unset)
        detector->RemoveModule(this);

    detector = nullptr;
}

// get module type name
std::string PRadHyCalModule::GetTypeName()
const
{
    return std::string(get_module_type_name(geometry.type));
}

// get sector name
std::string PRadHyCalModule::GetSectorName()
const
{
    return std::string(get_sector_name(geometry.sector));
}

//============================================================================//
// Public Static Member Functions                                             //
//============================================================================//

// convert name to primex id, it is highly specific for HyCal setup
int PRadHyCalModule::name_to_primex_id(const std::string &name)
{
    try {
        // lead tungstate module
        if(name.at(0) == 'W' || name.at(0) == 'w')
            return std::stoi(name.substr(1)) + 1000;

        // lead glass module
        if(name.at(0) == 'G' || name.at(0) == 'g')
            return std::stoi(name.substr(1));

    } catch(std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    // unknown module
    std::cerr << "Cannot auto determine id from mdoule" << name << std::endl;
    return -1;
}

// get sector position information from primex id
void PRadHyCalModule::get_sector_info(int pid, int &sector, int &row, int &col)
{
    // calculate geometry information
    if(pid > 1000) { // crystal module
        pid -= 1001;
        sector = (int)Center;
        row = pid/34 + 1;
        col = pid%34 + 1;
    } else { // lead glass module
        pid -= 1;
        int g_row = pid/30 + 1;
        int g_col = pid%30 + 1;
        // there are 4 sectors for lead glass
        // top sector
        if(g_col <= 24 && g_row <= 6) {
            sector = (int)Top;
            row = g_row;
            col = g_col;
        }
        // right sector
        if(g_col > 24 && g_row <= 24) {
            sector = (int)Right;
            row = g_row;
            col = g_col - 24;
        }
        // bottom sector
        if(g_col > 6 && g_row > 24) {
            sector = (int)Bottom;
            row = g_row - 24;
            col = g_col - 6;
        }
        // left sector
        if(g_col <= 6 && g_row > 6) {
            sector = (int)Left;
            row = g_row - 6;
            col = g_col;
        }
    }
}

// get enum ModuleType by its name
int PRadHyCalModule::get_module_type(const char *name)
{
    for(int i = 0; i < (int)Max_ModuleType; ++i)
        if(strcmp(name, __module_type_list[i]) == 0)
            return i;

    std::cerr << "PRad HyCal Module Error: Cannot find type " << name
              << ", please check the definition in PRadHyCalModule."
              << std::endl;
    // not found
    return -1;
}

// get enum HyCalSector by its name
int PRadHyCalModule::get_sector_id(const char *name)
{
    for(int i = 0; i < (int)Max_HyCalSector; ++i)
        if(strcmp(name, __hycal_sector_list[i]) == 0)
            return i;

    std::cerr << "PRad HyCal Module Error: Cannot find sector " << name
              << ", please check the definition in PRadHyCalModule."
              << std::endl;
    // not found
    return -1;
}

// get name of ModuleType
const char *PRadHyCalModule::get_module_type_name(int type)
{
    if(type < 0 || type >= (int)Max_ModuleType)
        return "Undefined";
    else
        return __module_type_list[type];
}

// get name of HyCalSector
const char *PRadHyCalModule::get_sector_name(int sec)
{
    if(sec < 0 || sec >= (int)Max_HyCalSector)
        return "Undefined";
    else
        return __hycal_sector_list[sec];
}



//============================================================================//
// Other Functions                                                            //
//============================================================================//

// output hycal module information
std::ostream &operator <<(std::ostream &os, const PRadHyCalModule &m)
{
    // name
    os << std::setw(8) << m.GetName()
    // type
       << std::setw(10) << m.GetTypeName()
    // geometry
       << std::setw(8) << m.GetSizeX()
       << std::setw(8) << m.GetSizeY()
       << std::setw(12) << m.GetX()
       << std::setw(12) << m.GetY()
    // sector info
       << std::setw(10) << m.GetSectorName()
       << std::setw(4) << m.GetRow()
       << std::setw(4) << m.GetColumn();

    return os;
}
