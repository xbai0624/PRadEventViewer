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
    if(x >= x_steps || y >= y_steps)
        return 0.;

    if(x < y)
        return profiles[type][y][x].frac;
    else
        return profiles[type][x][y].frac;
}

float PRadClusterProfile::GetError(int type, int x, int y)
const
{
    if(x >= x_steps || y >= y_steps)
        return 0.;

    if(x < y)
        return profiles[type][y][x].err;
    else
        return profiles[type][x][y].err;
}

const PRadClusterProfile::Profile &PRadClusterProfile::GetProfile(int type, int x, int y)
const
{
    if(x < y)
        return profiles[type][y][x];
    else
        return profiles[type][x][y];
}

