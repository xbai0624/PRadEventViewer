//============================================================================//
// EPICS (Experimental Physics and Industrial Control System)                 //
// The EPICS class that extract and store EPICS information from experimental //
// Data                                                                       //
//                                                                            //
// Chao Peng                                                                  //
// 11/18/2016                                                                 //
//============================================================================//

#include "PRadEPICSytem.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

#define EPICS_UNDEFINED_VALUE -9999.9

void PRadEPICSystem::PRadEPICSystem(const std::string &path)
{
    ReadEPICSMap(path);
}

void PRadEPICSystem::~PRadEPICSystem()
{
    // place holder
}

void PRadEPICSystem::AddChannel(const string &name)
{

}

void PRadEPICSystem::RegisterChannel(const std::string &name,
                                     const uint32_t &id,
                                     const float &value)
{
    if(id >= (uint32_t)epics_values.size())
    {
        epics_values.resize(id + 1, EPICS_UNDEFINED_VALUE);
    }

    epics_map[name] = id;
    epics_values.at(id) = value;
}

float PRadEPICSystem::GetEPICSValue(const string &name)
{
    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        cerr << "Data Handler: Did not find EPICS channel " << name << endl;
        return EPICS_UNDEFINED_VALUE;
    }

    return epics_values.at(it->second);
}

float PRadEPICSystem::GetEPICSValue(const string &name, const int &index)
{
    if((unsigned int)index >= energyData.size())
        return GetEPICSValue(name);

    return GetEPICSValue(name, energyData.at(index));
}

float PRadEPICSystem::GetEPICSValue(const string &name, const EventData &event)
{
    float result = EPICS_UNDEFINED_VALUE;

    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        cerr << "Data Handler: Did not find EPICS channel " << name << endl;
        return result;
    }

    uint32_t channel_id = it->second;
    int event_number = event.event_number;

    // find the epics event before this event
    for(size_t i = 0; i < epicsData.size(); ++i)
    {
        if(epicsData.at(i).event_number >= event_number) {
            if(i > 0) result = epicsData.at(i-1).values.at(channel_id);
            break;
        }
    }

    return result;
}

vector<epics_ch> PRadEPICSystem::GetSortedEPICSList()
{
    vector<epics_ch> epics_list;

    for(auto &ch : epics_map)
    {
        float value = epics_values.at(ch.second);
        epics_list.emplace_back(ch.first, ch.second, value);
    }

    sort(epics_list.begin(), epics_list.end(), [](const epics_ch &a, const epics_ch &b) {return a.id < b.id;});

    return epics_list;
}

void PRadEPICSystem::PrintOutEPICS()
{
    vector<epics_ch> epics_list = GetSortedEPICSList();

    for(auto &ch : epics_list)
    {
        cout << ch.name << ": " << epics_values.at(ch.id) << endl;
    }
}

void PRadEPICSystem::PrintOutEPICS(const string &name)
{
    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        cout << "Did not find the EPICS channel "
             << name << endl;
        return;
    }

    cout << name << ": " << epics_values.at(it->second) << endl;
}

void PRadEPICSystem::SaveEPICSChannels(const string &path)
{
    ofstream out(path);

    if(!out.is_open()) {
        cerr << "Cannot open file "
             << "\"" << path << "\""
             << " to save EPICS channels!"
             << endl;
        return;
    }

    vector<epics_ch> epics_list = GetSortedEPICSList();

    for(auto &ch : epics_list)
    {
        out << ch.name << endl;
    }

    out.close();
}

EPICSData &PRadEPICSystem::GetEPICSEvent(const unsigned int &index)
{
    if(index >= epicsData.size()) {
        return epicsData.back();
    } else {
        return epicsData.at(index);
    }
}

void PRadEPICSystem::ReadEPICSMap(const string &path)
{
    ConfigParser c_parser;

    if(!c_parser.ReadFile(path)) {
        cout << "WARNING: Fail to open EPICS channel file "
             << "\"" << path << "\""
             << ", no EPICS channel created!"
             << endl;
        return;
    }

    string name;
    float initial_value = EPICS_UNDEFINED_VALUE;

    while(c_parser.ParseLine())
    {
        if(c_parser.NbofElements() == 1) {
            name = c_parser.TakeFirst();
            if(epics_map.find(name) == epics_map.end()) {
                epics_map[name] = epics_values.size();
                epics_values.push_back(initial_value);
            } else {
                cout << "Duplicated epics channel " << name
                     << ", its channel id is " << epics_map[name]
                     << endl;
            }
        } else {
            cout << "Unrecognized input format in  epics channel file, skipped one line!"
                 << endl;
        }
    }

};
