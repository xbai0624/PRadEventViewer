#ifndef PRAD_HYCAL_MODULE_H
#define PRAD_HYCAL_MODULE_H

#include <string>
#include <iostream>

class PRadHyCalDetector;

class PRadHyCalModule
{
public:
    enum ModuleType
    {
        // undefined
        Undef_ModuleType = -1,
        // normal types
        PbGlass = 0,
        PbWO4 = 1,
        // max number of types
        Max_ModuleType,
    };
    enum HyCalSector
    {
        // undefined
        Undef_HyCalSector = -1,
        // normal sectors
        Center = 0,
        Top = 1,
        Right = 2,
        Bottom = 3,
        Left = 4,
        // max number of sectors
        Max_HyCalSector,
    };

    struct Geometry
    {
        int type;
        double size_x;
        double size_y;
        double x;
        double y;
        int sector;
        int row;
        int column;

        Geometry()
        : type(Undef_ModuleType), size_x(0), size_y(0), x(0), y(0),
          sector(Undef_HyCalSector), row(0), column(0)
        {};

        Geometry(int t, double sx, double sy, double pos_x, double pos_y)
        : type(ModuleType(t)), size_x(sx), size_y(sy), x(pos_x), y(pos_y),
          sector(Undef_HyCalSector), row(0), column(0)
        {};
    };

public:
    // constructors
    PRadHyCalModule(int pid,
                    const Geometry &geo = Geometry(),
                    PRadHyCalDetector *det = nullptr);
    PRadHyCalModule(const std::string &name,
                    const Geometry &geo = Geometry(),
                    PRadHyCalDetector *det = nullptr);
    PRadHyCalModule(const std::string &n,
                    int type, double size_x, double size_y, double x, double y,
                    PRadHyCalDetector *det = nullptr);

    // copy/move constructors
    PRadHyCalModule(const PRadHyCalModule &that);
    PRadHyCalModule(PRadHyCalModule &&that);

    // destructor
    virtual ~PRadHyCalModule();

    // asignment operators
    PRadHyCalModule &operator =(const PRadHyCalModule &rhs);
    PRadHyCalModule &operator =(PRadHyCalModule &&rhs);

    // set members
    void SetDetector(PRadHyCalDetector *det, bool force_set = false);
    void UnsetDetector(bool force_unset = false);
    void SetGeometry(const Geometry &geo) {geometry = geo;};

    // check type
    bool IsHyCalModule() const {return (geometry.type == PbGlass) || (geometry.type == PbWO4);};
    bool IsLeadTungstate() const {return geometry.type == PbWO4;};
    bool IsLeadGlass() const {return geometry.type == PbGlass;};

    // get members
    unsigned short GetID() const {return id;};
    int GetType() const {return geometry.type;};
    std::string GetTypeName() const;
    double GetX() const {return geometry.x;};
    double GetY() const {return geometry.y;};
    double GetSizeX() const {return geometry.size_x;};
    double GetSizeY() const {return geometry.size_y;};
    int GetSectorID() const {return geometry.sector;};
    std::string GetSectorName() const;
    int GetRow() const {return geometry.row;};
    int GetColumn() const {return geometry.column;};
    const Geometry &GetGeometry() const {return geometry;};
    const std::string &GetName() const {return name;};

    // compare operator
    bool operator < (const PRadHyCalModule &rhs) const
    {
        return id < rhs.id;
    }

public:
    // static functions
    static int name_to_primex_id(const std::string &name);
    static void get_sector_info(int pid, int &sec, int &row, int &col);
    static int get_module_type(const char *name);
    static int get_sector_id(const char *name);
    static const char *get_module_type_name(int type);
    static const char *get_sector_name(int sec);

private:
    PRadHyCalDetector *detector;
    std::string name;
    int id;
    Geometry geometry;
};

std::ostream &operator <<(std::ostream &os, const PRadHyCalModule &m);
#endif
