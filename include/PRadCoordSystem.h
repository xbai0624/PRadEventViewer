#ifndef PRAD_DET_COOR_H
#define PRAD_DET_COOR_H

#include <string>
#include <vector>
#include <iostream>
#include <map>

class PRadCoordSystem
{
public:
    enum CoordinateType
    {
        GEM1 = 0,
        GEM2,
        HyCal,
        Max_CoordinateType
    };
    std::string CoordTypeName[Max_CoordinateType] = {"PRadGEM1", "PRadGEM2", "HyCal"};

    struct Offsets
    {
        int run_number; // associated run
        double x_ori;   // origin x
        double y_ori;   // origin y
        double z_ori;   // origin z
        double theta_x; // tilting angle on x axis
        double theta_y; // tilting angle on y axis
        double theta_z; // tilting angle on z axis

        Offsets()
        : run_number(0), x_ori(0), y_ori(0), z_ori(0), theta_x(0), theta_y(0), theta_z(0)
        {};
        Offsets(int r, double x, double y, double z)
        : run_number(r), x_ori(x), y_ori(y), z_ori(z), theta_x(0), theta_y(0), theta_z(0)
        {};
        Offsets(int r, double x, double y, double z, double tx, double ty, double tz)
        : run_number(r), x_ori(x), y_ori(y), z_ori(z), theta_x(tx), theta_y(ty), theta_z(tz)
        {};
    };

public:
    PRadCoordSystem(const std::string &path = "", const int &run = 0);
    virtual ~PRadCoordSystem();

    void LoadCoordData(const std::string &path, const int &run = 0);
    void SaveCoordData(const std::string &path);

    const std::vector<Offsets> &GetCurrentOffsets() const {return current_offsets;};

    void Transform(CoordinateType type, double &x, double &y, double &z);

protected:
    // offsets data, run number as key, order is important, thus use map instead of hash map
    std::map<int, std::vector<Offsets>> offsets_data;
    std::vector<Offsets> current_offsets;
};

std::ostream &operator << (std::ostream &os, const PRadCoordSystem::Offsets &off);
#endif
