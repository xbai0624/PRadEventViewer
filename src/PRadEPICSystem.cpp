//============================================================================//
// EPICS (Experimental Physics and Industrial Control System)                 //
// The EPICS class that extract and store EPICS information from experimental //
// Data                                                                       //
//                                                                            //
// Chao Peng                                                                  //
// 11/18/2016                                                                 //
//============================================================================//

#include "PRadEPICSystem.h"
#include "ConfigParser.h"
#include "canalib.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

#define EPICS_UNDEFINED_VALUE -9999.9

PRadEPICSystem::PRadEPICSystem(const std::string &path)
{
    ReadMap(path);
}

PRadEPICSystem::~PRadEPICSystem()
{
    // place holder
}

void PRadEPICSystem::Reset()
{
    epics_data = std::deque<EPICS_Data>();

    for(auto &value : epics_values)
    {
        value = EPICS_UNDEFINED_VALUE;
    }
}

void PRadEPICSystem::ReadMap(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;

    if(!c_parser.ReadFile(path)) {
        std::cout << "PRad EPICS Warning: Failed to open map file "
                  << "\"" << path << "\""
                  << ", no EPICS channel created!"
                  << std::endl;
        return;
    }

    while(c_parser.ParseLine())
    {
        if(!c_parser.CheckElements(1))
            continue;

        AddChannel(c_parser.TakeFirst());
    }
}

void PRadEPICSystem::AddChannel(const std::string &name)
{
    auto it = epics_map.find(name);

    if(it == epics_map.end()) {
        epics_map[name] = epics_values.size();
        epics_values.push_back(EPICS_UNDEFINED_VALUE);
    } else {
        std::cout << "PRad EPICS Warning: Failed to add duplicated channel "
                  << name << ", its channel id is " << it->second
                  << std::endl;
    }
}

void PRadEPICSystem::AddChannel(const std::string &name, uint32_t id, float value)
{
    if(id >= (uint32_t)epics_values.size())
    {
        epics_values.resize(id + 1, EPICS_UNDEFINED_VALUE);
    }

    epics_map[name] = id;
    epics_values.at(id) = value;
}

void PRadEPICSystem::UpdateChannel(const std::string &name, const float &value)
{
    auto it = epics_map.find(name);
    if(it != epics_map.end()) {
        epics_values[it->second] = value;
    }
}

void PRadEPICSystem::FillRawData(const char *data)
{
    ConfigParser c_parser;
    // using config parser to deal with text buffer
    c_parser.ReadBuffer((const char*) data);

    float number;
    std::string name;
    while(c_parser.ParseLine())
    {
        // expect 2 elements for each line
        // channel_name  channel_value
        if(c_parser.NbofElements() == 2) {
            c_parser >> number >> name;
            UpdateChannel(name, number);
        }
    }
}

void PRadEPICSystem::AddEvent(EPICS_Data &&data)
{
    epics_data.emplace_back(data);
}

void PRadEPICSystem::AddEvent(const EPICS_Data &data)
{
    epics_data.push_back(data);
}

void PRadEPICSystem::SaveData(const int &event_number, bool online)
{
    if(online && epics_data.size())
        epics_data.pop_front();

    epics_data.emplace_back(event_number, epics_values);
}

float PRadEPICSystem::GetValue(const std::string &name)
const
{
    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        std::cout << "PRad EPICS Warning: Did not find EPICS channel "
                  << name
                  << std::endl;
        return EPICS_UNDEFINED_VALUE;
    }

    return epics_values.at(it->second);
}

float PRadEPICSystem::FindValue(int evt, const std::string &name)
const
{
    float result = EPICS_UNDEFINED_VALUE;

    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        std::cerr << "PRad EPICS Warning: Did not find EPICS channel "
                  << name
                  << std::endl;
        return EPICS_UNDEFINED_VALUE;
    }

    uint32_t channel_id = it->second;

    auto interval = cana::binary_search_interval(epics_data.begin(), epics_data.end(), evt);

    // found the epics event that just before evt, and it has that channel
    if((interval.first != epics_data.end()) &&
       (interval.first->values.size() > channel_id)) {
        result = interval.first->values.at(channel_id);
    }

    return result;
}

std::vector<EPICSChannel> PRadEPICSystem::GetSortedList()
const
{
    std::vector<EPICSChannel> epics_list;

    for(auto &ch : epics_map)
    {
        float value = epics_values.at(ch.second);
        epics_list.emplace_back(ch.first, ch.second, value);
    }

    std::sort(epics_list.begin(), epics_list.end(),
              [] (const EPICSChannel &a, const EPICSChannel &b)
              {return a.id < b.id;});

    return epics_list;
}

void PRadEPICSystem::SaveMap(const std::string &path)
const
{
    std::ofstream out(path);

    if(!out.is_open()) {
        std::cerr << "PRad EPICS Error: Cannot open file "
                  << "\"" << path << "\""
                  << " to save EPICS channels!"
                  << std::endl;
        return;
    }

    std::vector<EPICSChannel> epics_list = GetSortedList();

    for(auto &ch : epics_list)
    {
        out << ch.name << std::endl;
    }

    out.close();
}

const EPICS_Data &PRadEPICSystem::GetEvent(const unsigned int &index)
const
throw(PRadException)
{
    if(epics_data.empty())
        throw PRadException("PRad EPICS Error", "empty data bank!");

    if(index >= epics_data.size())
        return epics_data.back();

    return epics_data.at(index);
}

int PRadEPICSystem::FindEvent(int evt)
const
{
    auto interval = cana::binary_search_interval(epics_data.begin(), epics_data.end(), evt);

    // found the epics event that just before evt, and it has that channel
    if(interval.first != epics_data.end())
        return (interval.first - epics_data.begin());

    return -1;
}