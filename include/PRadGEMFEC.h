#ifndef PRAD_GEM_FEC_H
#define PRAD_GEM_FEC_H

#include <string>
#include <vector>
#include <unordered_map>
#include "PRadGEMAPV.h"

class PRadGEMFEC
{
public:
    PRadGEMFEC(const int &i, const std::string &p)
    : id(i), ip(p)
    {};
    virtual ~PRadGEMFEC();

    void AddAPV(PRadGEMAPV *apv);
    void RemoveAPV(const int &id);
    void SortAPVList();
    PRadGEMAPV *GetAPV(const int &id);
    std::vector<PRadGEMAPV *> &GetAPVList() {return adc_list;};
    void FitPedestal();
    void ClearAPVData();
    void CollectZeroSupHits(std::vector<GEM_Data> &hits);
    void Clear();

    int id;
    std::string ip;
    std::unordered_map<int, PRadGEMAPV*> adc_map;
    std::vector<PRadGEMAPV *> adc_list;
};

#endif
