#ifndef PRAD_EPIC_SYSTEM_H
#define PRAD_EPIC_SYSTEM_H

#include <string>
#include <unordered_map>
#include "PRadEventStruct.h"
#include "PRadException.h"

// epics channel
struct epics_ch
{
    std::string name;
    uint32_t id;
    float value;

    epics_ch(const std::string &n, const uint32_t &i, const float &v)
    : name(n), id(i), value(v)
    {};
};

class PRadEPICSystem
{
public:
    PRadEPICSystem(const std::string &path);
    virtual ~PRadEPICSystem();

    // read channel file
    void ReadEPICSMap(const std::string &path);
    // add channel
    void AddChannel(const std::string &name, const uint32_t &id, const float &value);

    void UpdateEPICS(const std::string &name, const float &value);

    EPICSData &GetEPICSEvent(const unsigned int &index);
    std::deque<EPICSData> &GetEPICSData() {return epicsData;};
    float GetEPICSValue(const std::string &name);
    float GetEPICSValue(const std::string &name, const int &index);
    float GetEPICSValue(const std::string &name, const EventData &event);
    void PrintOutEPICS();
    void PrintOutEPICS(const std::string &name);

    std::vector<epics_ch> GetSortedEPICSList();
    void SaveEPICSChannels(const std::string &path);

private:
    // data related
    std::unordered_map<std::string, uint32_t> epics_map;
    std::vector<float> epics_values;
    std::deque<EPICSData> epicsData;
};

#endif
