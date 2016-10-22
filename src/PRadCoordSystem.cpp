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

std::ostream &operator << (std::ostream &os, const PRadCoordSystem::Offsets &off)
{
    return os << std::setw(12) << off.x_ori
              << std::setw(12) << off.y_ori
              << std::setw(12) << off.z_ori
              << std::setw(12) << off.theta_x
              << std::setw(12) << off.theta_y
              << std::setw(12) << off.theta_z;
}

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

    offsets_data.clear();

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
        for(; index < (size_t) Max_CoordinateType; ++index)
        {
            if(det_name.find(CoordTypeName[index]) != std::string::npos)
                break;
        }

        if(index >= (size_t) Max_CoordinateType) {
            std::cout << "PRad Coord System Warning: Unrecognized detector "
                      << det_name << ", skipped reading its offsets."
                      << std::endl;
            continue;
        }
        Offsets new_off(run, x, y, z, theta_x, theta_y, theta_z);

        auto it = offsets_data.find(run);
        if(it == offsets_data.end()) {
            // create a new entry
            std::vector<Offsets> new_entry((size_t)Max_CoordinateType);
            new_entry[index] = new_off;

            offsets_data[run] = new_entry;
        } else {
            it->second[index] = new_off;
        }
    }

    auto  it = offsets_data.find(chosen_run);
    if((it == offsets_data.end()) && offsets_data.size()) {
    // set the first one as default offset
        current_offsets = offsets_data.begin()->second;
    } else {
        current_offsets = it->second;
    }
}

void PRadCoordSystem::SaveCoordData(const std::string &path)
{
    std::ofstream output(path);

    // output headers
    output << "# The least run will be chosen as the default offset" << std::endl
           << "# Lists the origin offsets of detectors to beam center and the tilting angles" << std::endl
           << "# Units are in mm and radian" << std::endl
           << "#" << std::setw(7) << "run"
           << std::setw(10) << "detector"
           << std::setw(12) << "x_origin"
           << std::setw(12) << "y_origin"
           << std::setw(12) << "z_origin"
           << std::setw(12) << "x_tilt"
           << std::setw(12) << "y_tilt"
           << std::setw(12) << "z_tilt"
           << std::endl;

    for(auto &it : offsets_data)
    {
        for(size_t i = 0; i < it.second.size(); ++i)
        {
            output << std::setw(8) << it.first
                   << std::setw(10) << CoordTypeName[i]
                   << it.second.at(i)
                   << std::endl;
        }
    }

    output.close();
}

// Transform the detector frame to beam frame
// it corrects the tilting angle first, and then correct origin
void PRadCoordSystem::Transform(PRadCoordSystem::CoordinateType type,
                                double &x, double &y, double &z)
{
    int index = (int)type;

    Offsets &coord = current_offsets[index];

    // firstly do the angle tilting
    // basic rotation matrix
    // Rx(a) = ( 1           0         0  )
    //         ( 0       cos(a)   -sin(a) )
    //         ( 0       sin(a)    cos(a) )
    y = y*cos(coord.theta_x) + z*sin(coord.theta_x);
    z = -y*sin(coord.theta_x) + z*cos(coord.theta_x);

    // Ry(a) = ( cos(a)      0     sin(a) )
    //         ( 0           1         0  )
    //         (-sin(a)      0     cos(a) )
    x = x*cos(coord.theta_y) - z*sin(coord.theta_y);
    z = x*sin(coord.theta_y) + z*cos(coord.theta_x);

    // Rz(a) = ( cos(a) -sin(a)        0  )
    //         ( sin(a)  cos(a)        0  )
    //         ( 0           0         1  )
    x = x*cos(coord.theta_z) + y*sin(coord.theta_z);
    y = -x*sin(coord.theta_z) + y*cos(coord.theta_z);

    // then correct the origin
    x += coord.x_ori;
    y += coord.y_ori;
    z += coord.z_ori;
}
