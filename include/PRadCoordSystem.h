#ifndef PRAD_DET_COOR_H
#define PRAD_DET_COOR_H

#include <string>
#include <vector>
#include <iostream>
#include <map>

class PRadCoordSystem
{
public:
    enum CoordType
    {
        GEM1 = 0,
        GEM2,
        HyCal,
        Max_CoordType
    };

    struct DetCoord
    {
        int run_number; // associated run
        int det_enum;   // detector index
        double x_ori;   // origin x
        double y_ori;   // origin y
        double z_ori;   // origin z
        double theta_x; // tilting angle on x axis
        double theta_y; // tilting angle on y axis
        double theta_z; // tilting angle on z axis

        DetCoord()
        : run_number(0), det_enum(0), x_ori(0), y_ori(0), z_ori(0), theta_x(0), theta_y(0), theta_z(0)
        {};
        DetCoord(int r, int i, double x, double y, double z)
        : run_number(r), det_enum(i), x_ori(x), y_ori(y), z_ori(z), theta_x(0), theta_y(0), theta_z(0)
        {};
        DetCoord(int r, int i, double x, double y, double z, double tx, double ty, double tz)
        : run_number(r), det_enum(i), x_ori(x), y_ori(y), z_ori(z), theta_x(tx), theta_y(ty), theta_z(tz)
        {};

        // these functions help to retrieve values in array or set values in array
        double get_dim_coord(int i)
        {
            if(i == 0) return x_ori;
            if(i == 1) return y_ori;
            if(i == 2) return z_ori;
            if(i == 3) return theta_x;
            if(i == 4) return theta_y;
            if(i == 5) return theta_z;
            return 0.;
        }

        void set_dim_coord(int i, double val)
        {
            if(i == 0) x_ori = val;
            if(i == 1) y_ori = val;
            if(i == 2) z_ori = val;
            if(i == 3) theta_x = val;
            if(i == 4) theta_y = val;
            if(i == 5) theta_z = val;
        }
    };

public:
    PRadCoordSystem(const std::string &path = "", const int &run = 0);
    virtual ~PRadCoordSystem();

    void LoadCoordData(const std::string &path, const int &run = 0);
    void SaveCoordData(const std::string &path);

    void SetCurrentCoord(const std::vector<DetCoord> &coords);
    void ChooseCoord(int run_number);

    const std::map<int ,std::vector<DetCoord>> &GetCoordsData() const {return coords_data;};
    std::vector<DetCoord> GetCurrentCoords() const {return current_coord;};

    void Transform(CoordType type, double &x, double &y, double &z) const;

protected:
    // offsets data, run number as key, order is important, thus use map instead of hash map
    std::map<int, std::vector<DetCoord>> coords_data;
    std::vector<DetCoord> current_coord;
};

std::ostream &operator << (std::ostream &os, const PRadCoordSystem::DetCoord &off);
const char *getCoordTypeName(int enumVal);

#endif
