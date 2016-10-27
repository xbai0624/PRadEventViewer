//============================================================================//
// GEM FEC class                                                              //
// FEC contains several APVs, but the APV won't be managed by FEC class       //
//                                                                            //
// Chao Peng                                                                  //
// 10/07/2016                                                                 //
//============================================================================//

#include "PRadGEMFEC.h"
#include "PRadGEMAPV.h"
#include <iostream>
#include <algorithm>



//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//

// constructor
PRadGEMFEC::PRadGEMFEC(const int &i, const std::string &p, const int &slots)
: id(i), ip(p)
{
    // open slots for inserting APVs
    adc_list.resize(slots, nullptr);
}

// copy constructor
PRadGEMFEC::PRadGEMFEC(const PRadGEMFEC &that)
: id(that.id), ip(that.ip)
{
    // open slots for inserting APVs
    adc_list.resize(that.adc_list.size(), nullptr);

    // APVs are not managed by FEC, so should not be copied
}

// move constructor
PRadGEMFEC::PRadGEMFEC(PRadGEMFEC &&that)
: id(that.id), ip(std::move(that.ip))
{
    // open slots for inserting APVs
    adc_list.resize(that.adc_list.size(), nullptr);

    // APVs are not managed by FEC, so should not be copied
}

// desctruct
// or
PRadGEMFEC::~PRadGEMFEC()
{
    Clear();
}

// copy constructor
PRadGEMFEC &PRadGEMFEC::operator =(const PRadGEMFEC &rhs)
{
    Clear();
    id = rhs.id;
    ip = rhs.ip;
    adc_list.resize(rhs.adc_list.size(), nullptr);
    return *this;
}

// move constructor
PRadGEMFEC &PRadGEMFEC::operator =(PRadGEMFEC &&rhs)
{
    Clear();
    id = rhs.id;
    ip = std::move(rhs.ip);
    adc_list.resize(rhs.adc_list.size(), nullptr);
    return *this;
}



//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

// change the capacity
void PRadGEMFEC::SetCapacity(int slots)
{
    // capacity cannot be negative
    if(slots < 0) slots = 0;

    if((size_t)slots < adc_list.size())
    {
        std::cout << "PRad GEM FEC Warning: Reduce the slots in FEC "
                  << id << " from " << adc_list.size() << " to " << slots
                  << ". All APVs beyond " << slots << " will be removed. "
                  << std::endl;

        for(size_t i = slots; i < adc_list.size(); ++i)
        {
            if(adc_list[i] != nullptr)
                adc_list[i]->DisconnectFEC();
        }
    }

    adc_list.resize(slots, nullptr);
}

// add an apv to fec
void PRadGEMFEC::AddAPV(PRadGEMAPV *apv, const int &slot)
{
    if(apv == nullptr)
        return;

    if((size_t)slot >= adc_list.size()) {
        std::cerr << "GEM FEC " << id
                  << ": Abort to add an apv to adc channel "
                  << apv->GetADCChannel()
                  << ", this FEC only has " << adc_list.size()
                  << " channels. (defined in PRadGEMFEC.h)"
                  << std::endl;
        return;
    }

    if(adc_list.at(slot) != nullptr) {
        std::cerr << "GEM FEC " << id
                  << ": Abort to add an apv to adc channel "
                  << apv->GetADCChannel()
                  << ", channel is occupied"
                  << std::endl;
        return;
    }

    adc_list[slot] = apv;
    apv->SetFEC(this, slot);
}

// remove apv in the slot
void PRadGEMFEC::RemoveAPV(const int &slot)
{
    if((size_t)slot >= adc_list.size())
        return;

    adc_list[slot] = nullptr;
}

// clear all the apvs in fec
void PRadGEMFEC::Clear()
{
    for(auto &adc : adc_list)
    {
        if(adc == nullptr)
            continue;
        adc->DisconnectFEC();
        adc = nullptr;
    }
}

// clear all the apvs' data
void PRadGEMFEC::ClearAPVData()
{
    for(auto &adc : adc_list)
    {
        if(adc != nullptr)
            adc->ClearData();
    }
}

// reset all the apvs' histograms
void PRadGEMFEC::ResetAPVHits()
{
    for(auto &adc : adc_list)
    {
        if(adc != nullptr)
            adc->ResetHitPos();
    }
}

// collect all the apvs' hits
void PRadGEMFEC::CollectZeroSupHits(std::vector<GEM_Data> &hits)
{
    for(auto &adc : adc_list)
    {
        if(adc != nullptr)
            adc->CollectZeroSupHits(hits);
    }
}

// fit all the apvs' pedestal histograms
void PRadGEMFEC::FitPedestal()
{
    for(auto &adc : adc_list)
    {
        if(adc != nullptr)
            adc->FitPedestal();
    }
}

// get the apv in the slot
PRadGEMAPV *PRadGEMFEC::GetAPV(const int &slot)
const
{
    if((size_t)slot >= adc_list.size())
        return nullptr;

    return adc_list[slot];
}

std::vector<PRadGEMAPV*> PRadGEMFEC::GetAPVList()
const
{
    std::vector<PRadGEMAPV*> result;
    for(auto &adc : adc_list)
    {
        if(adc != nullptr)
            result.push_back(adc);
    }
    return result;
}
