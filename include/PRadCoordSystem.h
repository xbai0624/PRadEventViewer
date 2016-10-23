#ifndef PRAD_DET_COOR_H
#define PRAD_DET_COOR_H

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include "PRadEventStruct.h"

class PRadCoordSystem
{
public:
    struct Point
    {
        float x;
        float y;
        float z;

        Point() : x(0), y(0), z(0)
        {};
        Point(float xi, float yi, float zi)
        : x(xi), y(yi), z(zi)
        {};
    };

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
        float x_ori;    // origin x
        float y_ori;    // origin y
        float z_ori;    // origin z
        float theta_x;  // tilting angle on x axis
        float theta_y;  // tilting angle on y axis
        float theta_z;  // tilting angle on z axis

        DetCoord()
        : run_number(0), det_enum(0), x_ori(0), y_ori(0), z_ori(0), theta_x(0), theta_y(0), theta_z(0)
        {};
        DetCoord(int r, int i, float x, float y, float z)
        : run_number(r), det_enum(i), x_ori(x), y_ori(y), z_ori(z), theta_x(0), theta_y(0), theta_z(0)
        {};
        DetCoord(int r, int i, float x, float y, float z, float tx, double ty, double tz)
        : run_number(r), det_enum(i), x_ori(x), y_ori(y), z_ori(z), theta_x(tx), theta_y(ty), theta_z(tz)
        {};

        // these functions help to retrieve values in array or set values in array
        float get_dim_coord(int i)
        {
            if(i == 0) return x_ori;
            if(i == 1) return y_ori;
            if(i == 2) return z_ori;
            if(i == 3) return theta_x;
            if(i == 4) return theta_y;
            if(i == 5) return theta_z;
            return 0.;
        }

        void set_dim_coord(int i, float val)
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

    // manipulate coordinates database
    void LoadCoordData(const std::string &path, const int &run = 0);
    void SaveCoordData(const std::string &path);
    void ChooseCoord(int run_number);

    // set members
    void SetCurrentCoord(const std::vector<DetCoord> &coords);

    // get members
    const std::map<int ,std::vector<DetCoord>> &GetCoordsData() const {return coords_data;};
    std::vector<DetCoord> GetCurrentCoords() const {return current_coord;};

    // basic transform functions
    void Transform(CoordType type, Point &p) const;
    void Transform(CoordType type, float &x, float &y, float &z) const;

    void TransformGEM(GEMHit *gem1, int nGEM1, GEMHit *gem2, int nGEM2) const;
    void TransformHyCal(HyCalHit *hit, int nHyCal) const;

    // basic projection functions
    void Projection(float &x, float &y, float &z,
                    const float &xi, const float &yi, const float &zi,
                    const float &zf) const;
    void Projection(Point &p, const Point &pi, const float &zf) const;
    void Projection(float &x, float &y, float &z, const float &zf) const;
    void Projection(float &x, float &y, float &z, const Point &pi, const float &zf) const;

public:
    // template functions
    // transform for clusters, accepts array
    template<class T>
    void Transform(CoordType type, T *t, int NCluster)
    const
    {
        for(int i = 0; i < NCluster; ++i)
        {
            Transform(type, t[i].x, t[i].y, t[i].z);
        }
    }

    // transform for clusters, accepts iterator
    template<class T_it>
    void Transform(CoordType type, T_it first, T_it last)
    const
    {
        for(T_it it = first; it != last; ++it)
        {
            Transform(type, (*it).x, (*it).y, (*it).z);
        }
    }

    // projection for clusters, accepts array
    template<class T>
    void Projection(T *t, int NCluster, const Point &pi, const float &zf)
    const
    {
        for(int i = 0; i < NCluster; ++i)
        {
            Projection(t[i].x, t[i].y, t[i].z, pi.x, pi.y, pi.z, zf);
        }
    }

    template<class T>
    void ProjectionToHyCal(T *t, int NCluster, const Point &pi = origin())
    const
    {
        float zf = current_coord[(int)HyCal].z_ori;
        for(int i = 0; i < NCluster; ++i)
        {
            Projection(t[i].x, t[i].y, t[i].z, pi.x, pi.y, pi.z, zf);
        }
    }


    // projection for clusters, accepts iterator
    template<class T_it>
    void Projection(T_it first, T_it last, const Point &pi, const float &zf)
    const
    {
        for(T_it it = first; it != last; ++it)
        {
            Projection((*it).x, (*it).y, (*it).z, pi.x, pi.y, pi.z, zf);
        }
    }

    template<class T_it>
    void Projection(T_it first, T_it last, const Point &pi = origin())
    {
        float zf = current_coord[(int)HyCal].z_ori;
        for(T_it it = first; it != last; ++it)
        {
            Projection((*it).x, (*it).y, (*it).z, pi.x, pi.y, pi.z, zf);
        }
    }


public:
    //static public members
    static Point beamLine(float z);
    static Point origin();

protected:
    // offsets data, run number as key, order is important, thus use map instead of hash map
    std::map<int, std::vector<DetCoord>> coords_data;
    std::vector<DetCoord> current_coord;
};

std::ostream &operator << (std::ostream &os, const PRadCoordSystem::DetCoord &off);
const char *getNameByCoordType(int enumVal);

#endif
