#ifndef PRAD_GEM_FEC_H
#define PRAD_GEM_FEC_H

#include <string>
#include <vector>
#include <unordered_map>
#include "PRadEventStruct.h"

// maximum channels in a FEC
#define FEC_CAPACITY 9

class PRadGEMAPV;

class PRadGEMFEC
{
public:
    // constructor
    PRadGEMFEC(const int &i, const std::string &p, const int &slots = FEC_CAPACITY);

    // copy/move constructors
    PRadGEMFEC(const PRadGEMFEC &that);
    PRadGEMFEC(PRadGEMFEC &&that);

    // descructor
    virtual ~PRadGEMFEC();

    // copy/move assignment operators
    PRadGEMFEC &operator =(const PRadGEMFEC &rhs);
    PRadGEMFEC &operator =(PRadGEMFEC &&rhs);

    // public member functions
    void SetCapacity(int slots);
    void AddAPV(PRadGEMAPV *apv, const int &slot);
    void RemoveAPV(const int &slot);
    void FitPedestal();
    void ClearAPVData();
    void ResetAPVHits();
    void CollectZeroSupHits(std::vector<GEM_Data> &hits);
    void Clear();

    // get parameters
    int GetID() const {return id;};
    const std::string &GetIP() const {return ip;};
    size_t GetCapacity() const {return adc_list.size();};
    PRadGEMAPV *GetAPV(const int &slot) const;
    std::vector<PRadGEMAPV*> GetAPVList() const;

private:
    int id;
    std::string ip;
    std::vector<PRadGEMAPV*> adc_list;
};

#endif
