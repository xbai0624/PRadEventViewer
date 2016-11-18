#ifndef PRAD_EPIC_SYSTEM_H
#define PRAD_EPIC_SYSTEM_H

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include "PRadEventStruct.h"
#include "PRadException.h"

// epics channel
struct EPICSChannel
{
    std::string name;
    uint32_t id;
    float value;

    EPICSChannel(const std::string &n, const uint32_t &i, const float &v)
    : name(n), id(i), value(v)
    {};
};

class PRadEPICSystem
{
public:
    PRadEPICSystem(const std::string &path);
    virtual ~PRadEPICSystem();

    void Reset();
    void ReadMap(const std::string &path);
    void SaveMap(const std::string &path) const;
    void AddChannel(const std::string &name);
    void AddChannel(const std::string &name, uint32_t id, float value);
    void UpdateChannel(const std::string &name, const float &value);
    void AddEvent(const EPICSData &data);
    void FillRawData(const char *buf);
    void SaveData(const int &event_number, bool online = false);

    std::vector<EPICSChannel> GetSortedList() const;
    const std::vector<float> &GetValues() const {return epics_values;};
    float GetValue(const std::string &name) const;
    float GetValue(const std::string &name, int evt) const;
    const EPICSData &GetEvent(const unsigned int &index) const throw(PRadException);
    const std::deque<EPICSData> &GetEventData() const {return epics_data;};
    unsigned int GetEventCount() const {return epics_data.size();};


private:
    // data related
    std::unordered_map<std::string, uint32_t> epics_map;
    std::vector<float> epics_values;
    std::deque<EPICSData> epics_data;
};

#endif
