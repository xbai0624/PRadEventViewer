//============================================================================//
// GEM plane class                                                            //
// A GEM plane is a component of GEM detector                                 //
// It is connected to several APV units                                       //
// GEM hits are collected and formed on plane level                           //
//                                                                            //
// Chao Peng, Xinzhan Bai                                                     //
// 10/07/2016                                                                 //
//============================================================================//

#include "PRadGEMPlane.h"
#include "PRadGEMAPV.h"
#include <iostream>
#include <iterator>
#include <algorithm>

#define PITCH 0.4
#define X_SHIFT 253.2

PRadGEMPlane::PRadGEMPlane()
: detector(nullptr), name("Undefined"), type(Plane_X), size(0.),
  connector(-1), orientation(0)
{
    // place holder
}

PRadGEMPlane::PRadGEMPlane(const std::string &n, const PlaneType &t, const double &s,
                           const int &c, const int &o, const int &d)
: detector(nullptr), name(n), type(t), size(s), connector(c), orientation(o), direction(d)
{
    apv_list.resize(c, nullptr);
}

PRadGEMPlane::PRadGEMPlane(PRadGEMDetector *det, const std::string &n, const PlaneType &t,
                           const double &s, const int &c, const int &o, const int &d)
: detector(det), name(n), type(t), size(s), connector(c), orientation(o), direction(d)
{
    apv_list.resize(c, nullptr);
}

PRadGEMPlane::~PRadGEMPlane()
{
    // place holder
    for(auto &apv : apv_list)
    {
        if(apv != nullptr)
            apv->DisconnectPlane();
    }
}

void PRadGEMPlane::SetCapacity(const int &c)
{
    if(c < connector)
    {
        std::cout << "PRad GEM Plane Warning: Reduce the connectors on plane "
                  << name << " from " << connector << " to " << c
                  << ". Thus it will lose the connection between APVs that beyond "
                  << c
                  << std::endl;
    }
    connector = c;

    apv_list.resize(c, nullptr);
}

void PRadGEMPlane::ConnectAPV(PRadGEMAPV *apv, const int &index)
{
    if(apv == nullptr)
        return;

    if((size_t)index >= apv_list.size()) {
        std::cout << "PRad GEM Plane Warning: Failed to connect plane " << name
                  << " with APV " << apv->GetAddress()
                  << ". Plane connectors are not enough, have " << connector
                  << ", this APV is to be connected at " << index
                  << std::endl;
        return;
    }

    if(apv_list[index] != nullptr) {
        std::cout << "PRad GEM Plane Warning: The connector " << index
                  << " of plane " << name << " is connected to APV " << apv->GetAddress()
                  << ", replace the connection."
                  << std::endl;
        return;
    }

    apv_list[index] = apv;
    apv->SetDetectorPlane(this, index);
}

void PRadGEMPlane::DisconnectAPV(const size_t &plane_index)
{
    if(plane_index >= apv_list.size())
        return;

    apv_list[plane_index] = nullptr;
}

std::vector<PRadGEMAPV*> PRadGEMPlane::GetAPVList()
{
    // since the apv list may contain nullptr,
    // only pack connected APVs and return
    std::vector<PRadGEMAPV*> result;

    for(const auto &apv : apv_list)
    {
        if(apv != nullptr)
            result.push_back(apv);
    }

    return result;
}

// unit is mm
// X plane did a shift to move the X origin to center hole
// origin shift: 550.4/2 - (44-pitch)/2 = 253.2 mm
double PRadGEMPlane::GetStripPosition(const int &plane_strip)
{
    double position;

    if(type == Plane_X) {
        position = -0.5*(size + 31*PITCH) + PITCH*plane_strip - X_SHIFT;
    } else {
        position = -0.5*(size - PITCH) + PITCH*plane_strip;
    }

    return direction*position;
}

double PRadGEMPlane::GetMaxCharge(const std::vector<float> &charges)
{
    if(!charges.size())
        return 0.;

    double result = charges.at(0);

    for(size_t i = 1; i < charges.size(); ++i)
    {
        if(result < charges.at(i))
            result = charges.at(i);
    }

    return result;
}

double PRadGEMPlane::GetIntegratedCharge(const std::vector<float> &charges)
{
    double result = 0.;

    for(auto &charge : charges)
        result += charge;

    return result;
}

void PRadGEMPlane::ClearPlaneHits()
{
    hit_list.clear();
}

// X plane needs to remove 16 strips at both ends, because they are floating
// This is a special setup for PRad GEMs, so not configurable
void PRadGEMPlane::AddPlaneHit(const int &plane_strip, const std::vector<float> &charges)
{
    if((type == Plane_X) &&
       ((plane_strip < 16) || (plane_strip > 1391)))
       return;

    hit_list.emplace_back(plane_strip, GetMaxCharge(charges));
}

void PRadGEMPlane::CollectAPVHits()
{
    ClearPlaneHits();

    for(auto &apv : apv_list)
    {
        if(apv != nullptr)
            apv->CollectZeroSupHits();
    }
}

