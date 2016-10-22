//============================================================================//
// Transform the HyCal and GEM to beam center frame                           //
//                                                                            //
// Weizhi XIong, Xinzhan Bai, Chao Peng                                       //
// 10/21/2016                                                                 //
//============================================================================//

#include "PRadCoordSystem.h"
#include "ConfigParser.h"
#include <cmath>
#include <fstream>
#include <iomanip>

static const char *CoordTypeName[] = {"PRadGEM1", "PRadGEM2", "HyCal", "Undefined"};

PRadCoordSystem::PRadCoordSystem(const std::string &path, const int &run)
{
    if(!path.empty())
        LoadCoordData(path, run);
}

PRadCoordSystem::~PRadCoordSystem()
{
    // place holder
}

// load coordinates data, the format should be
// run_number, detector_name, origin_x, origin_y, origin_z, theta_x, theta_y, theta_z
// it reads the coordinates of the origin in the detector frame and its tilting angles
void PRadCoordSystem::LoadCoordData(const std::string &path, const int &chosen_run)
{
    ConfigParser c_parser;

    if(!c_parser.OpenFile(path)) {
        std::cerr << "PRad Coord System Error: Cannot open data file "
                  << "\"" << path << "\"."
                  << std::endl;
        return;
    }

    coords_data.clear();
    current_coord.clear();

    while(c_parser.ParseLine())
    {
        if(c_parser.NbofElements() < 8)
            continue;

        int run;
        std::string det_name;
        double x, y, z, theta_x, theta_y, theta_z;

        c_parser >> run >> det_name
                 >> x >> y >> z >> theta_x >> theta_y >> theta_z;

        size_t index = 0;
        for(; index < (size_t) Max_CoordType; ++index)
        {
            if(det_name.find(CoordTypeName[index]) != std::string::npos)
                break;
        }

        if(index >= (size_t) Max_CoordType) {
            std::cout << "PRad Coord System Warning: Unrecognized detector "
                      << det_name << ", skipped reading its offsets."
                      << std::endl;
            continue;
        }
        DetCoord new_off(run, index, x, y, z, theta_x, theta_y, theta_z);

        auto it = coords_data.find(run);
        if(it == coords_data.end()) {
            // create a new entry
            std::vector<DetCoord> new_entry((size_t)Max_CoordType);
            new_entry[index] = new_off;

            coords_data[run] = new_entry;
        } else {
            it->second[index] = new_off;
        }
    }

    ChooseCoord(chosen_run);
}

void PRadCoordSystem::SaveCoordData(const std::string &path)
{
    std::ofstream output(path);

    // output headers
    output << "# The least run will be chosen as the default offset" << std::endl
           << "# Lists the origin offsets of detectors to beam center and the tilting angles" << std::endl
           << "# Units are in mm and mradian" << std::endl
           << "#" << std::setw(7) << "run"
           << std::setw(10) << "detector"
           << std::setw(12) << "x_origin"
           << std::setw(12) << "y_origin"
           << std::setw(12) << "z_origin"
           << std::setw(12) << "x_tilt"
           << std::setw(12) << "y_tilt"
           << std::setw(12) << "z_tilt"
           << std::endl;

    for(auto &it : coords_data)
    {
        for(auto &coord : it.second)
        {
            output << coord << std::endl;
        }
    }

    output.close();
}

// choose the coordinate offsets from the database
void PRadCoordSystem::ChooseCoord(int run_number)
{
    if(coords_data.empty()) {
        std::cerr << "PRad Coord System Error: Database is empty, make sure you "
                  << "have loaded the correct coordinates data."
                  << std::endl;
        return;
    }

    // choose the default run
    if(run_number == 0) {
        current_coord = coords_data.begin()->second;
        return;
    }

    auto it = coords_data.find(run_number);
    if((it == coords_data.end())) {
        current_coord = coords_data.begin()->second;
        std::cout << "PRad Coord System Warning: Cannot find run " << run_number
                  << " in the current database, choose the default run "
                  << coords_data.begin()->first
                  << std::endl;
    } else {
        current_coord = it->second;
    }
}

void PRadCoordSystem::SetCurrentCoord(const std::vector<PRadCoordSystem::DetCoord> &coords)
{
    current_coord = coords;

    current_coord.resize((int)Max_CoordType);

    coords_data[current_coord.begin()->run_number] = current_coord;
}

// Transform the detector frame to beam frame
// it corrects the tilting angle first, and then correct origin
void PRadCoordSystem::Transform(PRadCoordSystem::CoordType type,
                                double &x, double &y, double &z)
const
{
    const DetCoord &coord = current_coord.at((int)type);

    // firstly do the angle tilting
    // basic rotation matrix
    // Rx(a) = ( 1           0         0  )
    //         ( 0       cos(a)   -sin(a) )
    //         ( 0       sin(a)    cos(a) )
    y = y*cos(coord.theta_x*0.001) + z*sin(coord.theta_x*0.001);
    z = -y*sin(coord.theta_x*0.001) + z*cos(coord.theta_x*0.001);

    // Ry(a) = ( cos(a)      0     sin(a) )
    //         ( 0           1         0  )
    //         (-sin(a)      0     cos(a) )
    x = x*cos(coord.theta_y*0.001) - z*sin(coord.theta_y*0.001);
    z = x*sin(coord.theta_y*0.001) + z*cos(coord.theta_x*0.001);

    // Rz(a) = ( cos(a) -sin(a)        0  )
    //         ( sin(a)  cos(a)        0  )
    //         ( 0           0         1  )
    x = x*cos(coord.theta_z*0.001) + y*sin(coord.theta_z*0.001);
    y = -x*sin(coord.theta_z*0.001) + y*cos(coord.theta_z*0.001);

    // then correct the origin
    x += coord.x_ori;
    y += coord.y_ori;
    z += coord.z_ori;
}

std::ostream &operator << (std::ostream &os, const PRadCoordSystem::DetCoord &det)
{
    return os << std::setw(8)  << det.run_number
              << std::setw(12) << CoordTypeName[det.det_enum]
              << std::setw(12) << det.x_ori
              << std::setw(12) << det.y_ori
              << std::setw(12) << det.z_ori
              << std::setw(8) << det.theta_x
              << std::setw(8) << det.theta_y
              << std::setw(8) << det.theta_z;
}

const char *getCoordTypeName(int enumVal)
{
    return CoordTypeName[enumVal];
}
