//============================================================================//
// A class to store the information about the HyCal cluster profile           //
// It is a singleton class to be shared among different clustering methods    //
//                                                                            //
// Chao Peng                                                                  //
// 11/22/2016                                                                 //
//============================================================================//

#include "PRadClusterProfile.h"
#include "PRadHyCalModule.h"
#include "ConfigParser.h"
#include <cmath>


PRadClusterProfile::PRadClusterProfile(int t, int x, int y)
: types(t), x_steps(x), y_steps(y)
{
    reserve();
}

PRadClusterProfile::~PRadClusterProfile()
{
    release();
}

void PRadClusterProfile::reserve()
{
    profiles = new Profile**[types];
    for(int i = 0; i < types; ++i)
    {
        profiles[i] = new Profile*[x_steps];
        for(int j = 0; j < x_steps; ++j)
        {
            profiles[i][j] = new Profile[y_steps];
        }
    }

}

void PRadClusterProfile::release()
{
    if(!profiles)
        return;

    for(int i = 0; i < types; ++i)
    {
        for(int j = 0; j < x_steps; ++j)
        {
            delete [] profiles[i][j], profiles[i][j] = nullptr;
        }
        delete [] profiles[i], profiles[i] = nullptr;
    }
    delete [] profiles, profiles = nullptr;

    types = 0;
    x_steps = 0;
    y_steps = 0;
}

void PRadClusterProfile::Resize(int t, int x, int y)
{
    release();
    types = t;
    x_steps = x;
    y_steps = y;
    reserve();
}

void PRadClusterProfile::Clear()
{
    for(int i = 0; i < types; ++i)
    {
        for(int j = 0; j < x_steps; ++j)
        {
            for(int k = 0; k < y_steps; ++k)
            {
                profiles[i][j][k] = Profile(0, 0);
            }
        }
    }
}

void PRadClusterProfile::LoadProfile(int type, const std::string &path)
{
    if(path.empty())
        return;

    if(type >= types) {
        std::cerr << "PRad Cluster Profile Error: Exceed current capacity, "
                  << "only has " << types << " types."
                  << std::endl;
        return;
    }

    Profile **profile = profiles[type];

    ConfigParser parser;
    if(!parser.OpenFile(path)) {
        std::cerr << "PRad Cluster Profile Error: File"
                  << " \"" << path << "\"  "
                  << "cannot be opened."
                  << std::endl;
        return;
    }

    int x, y;
    float val, err;
    while(parser.ParseLine())
    {
        if(!parser.CheckElements(4))
            continue;

        parser >> x >> y >> val >> err;
        if(x >= x_steps || y >= y_steps) {
            std::cout << "PRad Cluster Profile Warning: step "
                      << "(" << x << ", " << y << ") "
                      << "exceeds current capacity, only supports up to "
                      << "(" << x_steps << ", " << y_steps << ")."
                      << std::endl;
            continue;
        }
        profile[x][y] = Profile(val, err);
    }
}

// x y is symmetric in profile
float PRadClusterProfile::GetFraction(int type, int x, int y)
const
{
    return GetProfile(type, x, y).frac;
}

float PRadClusterProfile::GetError(int type, int x, int y)
const
{
    return GetProfile(type, x, y).err;
}

typedef PRadClusterProfile::Profile CProfile;

const CProfile &PRadClusterProfile::GetProfile(int type, int x, int y)
const
{
    if(x >= x_steps || y >= y_steps)
        return empty_prof;

    if(x < y)
        return profiles[type][y][x];
    else
        return profiles[type][x][y];
}

// highly specific to HyCal geometry
// TODO generalize it according to the module list read in
#define PWO_X_BOUNDARY 353.09
#define PWO_Y_BOUNDARY 352.75
static float __cp_size_x[2] = {38.15, 20.77};
static float __cp_size_y[2] = {38.15, 20.75};
static float __cp_boundary[4] = {PWO_Y_BOUNDARY, PWO_X_BOUNDARY,
                                 -PWO_Y_BOUNDARY, -PWO_X_BOUNDARY};
static bool __cp_x_boundary[4] = {false, true, false, true};

inline int __cp_get_sector(const float &x, const float &y)
{
    if(y > PWO_Y_BOUNDARY && x <= PWO_X_BOUNDARY)
        return 1; // top

    if(x > PWO_X_BOUNDARY && y > -PWO_X_BOUNDARY)
        return 2; // right

    if(y <= -PWO_Y_BOUNDARY && x > -PWO_X_BOUNDARY)
        return 3; // bottom

    if(x <= -PWO_X_BOUNDARY && y <= PWO_Y_BOUNDARY)
        return 4; // left

    return 0; // center
}

const CProfile &PRadClusterProfile::GetProfile(const float &x1, const float &y1,
                                               const float &x2, const float &y2)
const
{
    // firstly, check which part the point belongs to
    // 0 means pwo module and 1,2,3,4 means lg module
    int sect1 = __cp_get_sector(x1, y1);
    int sect2 = __cp_get_sector(x2, y2);

    int type1 = (sect1 == 0)? PRadHyCalModule::PbWO4 : PRadHyCalModule::PbGlass;
    int type2 = (sect2 == 0)? PRadHyCalModule::PbWO4 : PRadHyCalModule::PbGlass;

    int dx, dy;
    // both belong to the same part
    if(type1 == type2) {
        dx = abs((x1 - x2)/__cp_size_x[type1] * 100.);
        dy = abs((y1 - y2)/__cp_size_y[type1] * 100.);
    // belong to different part
    } else {
        // determine the line that connects the two points
        float k = (y1 - y2)/(x1 - x2);
        float b = y1 - k*x1;

        // determine which boundary the line is crossing
        int sect = abs(sect1 - sect2);
        float boundary = __cp_boundary[sect - 1];
        bool x_boundary = __cp_x_boundary[sect - 1];

        // get the intersect point
        float inter_x, inter_y;
        if(x_boundary) {
            inter_x = boundary;
            inter_y = k*boundary + b;
        } else {
            inter_y = boundary;
            inter_x = (boundary - b)/k;
        }

        dx =  abs((x1 - inter_x)/__cp_size_x[type1]*100.)
            + abs((x2 - inter_x)/__cp_size_x[type2]*100.);
        dy =  abs((y1 - inter_y)/__cp_size_y[type1]*100.)
            + abs((y2 - inter_y)/__cp_size_y[type2]*100.);
    }

    return GetProfile(type1, dx, dy);
}

