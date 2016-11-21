#ifndef PRAD_CLUSTER_PROFILE_H
#define PRAD_CLUSTER_PROFILE_H

#include <string>

class PRadHyCalModule;

class PRadClusterProfile
{
public:
    struct Profile
    {
        float frac;
        float err;

        Profile() : frac(0), err(0) {};
        Profile(float f, float e) : frac(f), err(e) {};
    };

public:
    static PRadClusterProfile &Instance()
    {
        static PRadClusterProfile instance;

        return instance;
    }

    // copy/move constructors
    PRadClusterProfile(const PRadClusterProfile &that) = delete;
    PRadClusterProfile(PRadClusterProfile &&that) = delete;

    virtual ~PRadClusterProfile();

    // copy/move assignment operators
    PRadClusterProfile &operator =(const PRadClusterProfile &rhs) = delete;
    PRadClusterProfile &operator =(PRadClusterProfile &&rhs) = delete;

    void Resize(int type, int xsize, int ysize);
    void Clear();
    void LoadProfile(int type, const std::string &path);
    static float GetFraction(int type, int x, int y);
    static float GetError(int type, int x, int y);
    static Profile &GetProfile(int type, int x, int y);

private:
    PRadClusterProfile(int type = 2, int xsize = 501, int ysize = 501);
    void reserve();
    void release();

private:
    int types;
    int x_steps;
    int y_steps;
    Profile ***profiles;
};

#endif
