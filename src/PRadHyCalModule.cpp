//============================================================================//
// Basic DAQ channel unit                                                     //
// It could be HyCal Module, LMS PMT or Scintillator                          //
// Chao Peng                                                                  //
// 02/17/2016                                                                 //
//============================================================================//

#include "PRadHyCalModule.h"
#include "PRadHyCalDetector.h"
#include "PRadADCChannel.h"
#include "PRadEventStruct.h"
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
: detector(det), daq_ch(nullptr), name(n), geometry(geo)
{
    id = name_to_primex_id(n);
}

PRadHyCalModule::PRadHyCalModule(int pid, const Geometry &geo, PRadHyCalDetector *det)
: detector(det), daq_ch(nullptr), id(pid), geometry(geo)
{
    if(geo.type == PbGlass)
        name = "G";
    if(geo.type == PbWO4)
        name = "W";

    name += std::to_string(id);
}

// copy constructor
PRadHyCalModule::PRadHyCalModule(const PRadHyCalModule &that)
: detector(nullptr), daq_ch(nullptr), name(that.name), id(that.id),
  geometry(that.geometry), cal_const(that.cal_const)
{
    // place holder
}

// move constructor
PRadHyCalModule::PRadHyCalModule(PRadHyCalModule &&that)
: detector(nullptr), daq_ch(nullptr), name(std::move(that.name)), id(that.id),
  geometry(that.geometry), cal_const(that.cal_const)
{
    // place holder
}

// destructor
PRadHyCalModule::~PRadHyCalModule()
{
    UnsetDetector();
    UnsetChannel();
}

// copy assignment operator
PRadHyCalModule &PRadHyCalModule::operator =(const PRadHyCalModule &rhs)
{
    name = rhs.name;
    id = rhs.id;
    geometry = rhs.geometry;
    cal_const = rhs.cal_const;
    return *this;
}

PRadHyCalModule &PRadHyCalModule::operator =(PRadHyCalModule &&rhs)
{
    name = std::move(rhs.name);
    id = rhs.id;
    geometry = rhs.geometry;
    cal_const = std::move(rhs.cal_const);
    return *this;
}

//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

// set detector
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
        detector->DisconnectModule(this, true);

    detector = nullptr;
}

// set daq channel
void PRadHyCalModule::SetChannel(PRadADCChannel *ch, bool force_set)
{
    if(ch == daq_ch)
        return;

    if(!force_set)
        UnsetChannel();

    daq_ch = ch;
}

// disconnect the daq channel
void PRadHyCalModule::UnsetChannel(bool force_unset)
{
    if(!daq_ch)
        return;

    if(!force_unset)
        daq_ch->UnsetModule(true);

    daq_ch = nullptr;
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

PRadTDCChannel *PRadHyCalModule::GetTDC()
const
{
    if(daq_ch)
        return daq_ch->GetTDC();
    return nullptr;
}

double PRadHyCalModule::GetEnergy()
const
{
    // did not connect to a adc_channel
    if(!daq_ch)
        return 0.;

    return cal_const.Calibration(daq_ch->GetReducedValue());
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
void PRadHyCalModule::hycal_info(int pid, int &sector, int &row, int &col, unsigned int &flag)
{
    // calculate geometry information
    flag = 0;

    if(pid > 1000) {
        // crystal module
        pid -= 1001;
        sector = (int)Center;
        row = pid/34 + 1;
        col = pid%34 + 1;

        // set flag
        SET_BIT(flag, kPWO);
        if(row <= 19 && row >= 16 && col <= 19 && row >= 16)
            SET_BIT(flag, kInnerBound);
        if(row == 1 || row == 34 || col == 1 || col == 34)
            SET_BIT(flag, kTransition);

    } else {
        // lead glass module
        pid -= 1;
        int g_row = pid/30 + 1;
        int g_col = pid%30 + 1;

        // set flag
        SET_BIT(flag, kLG);
        if(g_row == 1 || g_row == 30 || g_col == 1 || g_col == 30)
            SET_BIT(flag, kOuterBound);

        // there are 4 sectors for lead glass
        // top sector
        if(g_col <= 24 && g_row <= 6) {
            sector = (int)Top;
            row = g_row;
            col = g_col;
            if(row == 6 && col >= 6)
                SET_BIT(flag, kTransition);
        }
        // right sector
        if(g_col > 24 && g_row <= 24) {
            sector = (int)Right;
            row = g_row;
            col = g_col - 24;
            if(col == 1 && row >= 6)
                SET_BIT(flag, kTransition);
        }
        // bottom sector
        if(g_col > 6 && g_row > 24) {
            sector = (int)Bottom;
            row = g_row - 24;
            col = g_col - 6;
            if(row == 1 && col < 20)
                SET_BIT(flag, kTransition);
        }
        // left sector
        if(g_col <= 6 && g_row > 6) {
            sector = (int)Left;
            row = g_row - 6;
            col = g_col;
            if(col == 6 && row < 20)
                SET_BIT(flag, kTransition);
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

double PRadHyCalModule::distance(const PRadHyCalModule &m1, const PRadHyCalModule &m2)
{
    double x_dis = m1.geometry.x - m2.geometry.x;
    double y_dis = m1.geometry.y - m2.geometry.y;
    return sqrt(x_dis*x_dis + y_dis*y_dis);
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
       << std::setw(8) << m.GetSizeZ()
       << std::setw(12) << m.GetX()
       << std::setw(12) << m.GetY()
       << std::setw(12) << m.GetZ()
    // sector info
       << std::setw(10) << m.GetSectorName()
       << std::setw(4) << m.GetRow()
       << std::setw(4) << m.GetColumn();

    return os;
}

