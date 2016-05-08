#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

// Multi threads to deal with several banks in an event simultaneously.
// Depends on the running evironment, you may have greatly improved
// performance to read large evio file.
//#define MULTI_THREAD

#ifdef MULTI_THREAD
#include <thread>
#include <mutex>
#endif

#include <unordered_map>
#include <deque>
#include <vector>
#include "datastruct.h"

using namespace std;

class PRadEvioParser;
class PRadDAQUnit;
class TH1I;
class TH1D;

// a simple hash function for DAQ configuration
namespace std {
    template <>
    struct hash<ChannelAddress>
    {
        size_t operator()(const ChannelAddress& cfg) const
        {
            // crate id is 1-6, slot is 1-26, channel is 0-63
            // thus they can be filled in a 16 bit word
            // [ 0 0 0 | 0 0 0 0 0 | 0 0 0 0 0 0 0 0 ]
            // [ crate |    slot   |     channel     ]
            // this simple hash ensures no collision at current setup
            // which means fast access
            return (cfg.crate << 13 | cfg.slot << 8 | cfg.channel);
        }
   };
}

typedef unordered_map< ChannelAddress, PRadDAQUnit* >::iterator daq_iter;
typedef unordered_map< string, PRadDAQUnit* >::iterator name_iter;
typedef unordered_map< string, vector< PRadDAQUnit* > >::iterator tdc_iter;

class PRadDataHandler
{
public:
    typedef struct {
        unsigned short id;
        unsigned short adcValue;
    } ChannelData;

    PRadDataHandler();
    virtual ~PRadDataHandler();
    void RegisterChannel(PRadDAQUnit *channel);
    void FeedData(ADC1881MData &adcData);
    void FeedData(GEMAPVData &gemData);
    void FeedData(CAENHVData &hvData);
    void ShowEvent(int idx = 0);
    int GetEventCount() {return energyData.size();};
    TH1D *GetEnergyHist() {return energyHist;};
    void Clear();
    void EndofThisEvent();
    void OnlineMode() {onlineMode = true;};
    void OfflineMode() {onlineMode = false;};
    void BuildChannelMap();
    PRadDAQUnit *FindChannel(const ChannelAddress &daqInfo);
    PRadDAQUnit *FindChannel(const string &name);
    PRadDAQUnit *FindChannel(const unsigned short &id);
    vector< PRadDAQUnit* > GetTDCGroup(string &name);
    vector< string > GetTDCGroupList();
    const vector< PRadDAQUnit* > &GetChannelList() {return channelList;};

private:
    double totalE;
    bool onlineMode;
#ifdef MULTI_THREAD
    mutex myLock;
#endif
    unordered_map< ChannelAddress, PRadDAQUnit* > map_daq;
    unordered_map< string, PRadDAQUnit* > map_name;
    unordered_map< string, vector< PRadDAQUnit* > > map_tdc;
    vector< PRadDAQUnit* > channelList;
    deque< vector< ChannelData > > energyData;
    vector < ChannelData > newEvent, lastEvent;
    TH1D *energyHist;
};

#endif
