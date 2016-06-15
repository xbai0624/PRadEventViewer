#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

// Multi threads to deal with several banks in an event simultaneously.
// Depends on the running evironment, you may have greatly improved
// performance to read large evio file.
#define MULTI_THREAD

#ifdef MULTI_THREAD
#include <thread>
#include <mutex>
#endif

#include <unordered_map>
#include <map>
#include <deque>
#include <fstream>
#include "datastruct.h"
#include "PRadEventStruct.h"
#include "PRadException.h"

class PRadEvioParser;
class PRadDAQUnit;
class PRadTDCGroup;
class TH1D;
class TH2I;

// a simple hash function for DAQ configuration
namespace std
{
    template <>
    struct hash<ChannelAddress>
    {
        size_t operator()(const ChannelAddress &cfg) const
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

// helper struct
struct epics_ch
{
    std::string name;
    size_t id;

    epics_ch(const std::string &n, const size_t &i)
    : name(n), id(i)
    {};
};

class PRadDataHandler
{
public:
    PRadDataHandler();
    virtual ~PRadDataHandler();

    // mode change
    void SetOnlineMode(const bool &mode);

    // add channels
    void AddChannel(PRadDAQUnit *channel);
    void AddTDCGroup(PRadTDCGroup *group);
    void RegisterChannel(PRadDAQUnit *channel);
    void BuildChannelMap();

    // get channels/lists
    PRadDAQUnit *GetChannel(const ChannelAddress &daqInfo);
    PRadDAQUnit *GetChannel(const std::string &name);
    PRadDAQUnit *GetChannel(const unsigned short &id);
    PRadTDCGroup *GetTDCGroup(const std::string &name);
    PRadTDCGroup *GetTDCGroup(const ChannelAddress &addr);
    const std::unordered_map< std::string, PRadTDCGroup *> &GetTDCGroupSet() {return map_name_tdc;};
    std::vector< PRadDAQUnit* > &GetChannelList() {return channelList;};
 
    // read config files
    void ReadTDCList(const std::string &path);
    void ReadChannelList(const std::string &path);
    void ReadPedestalFile(const std::string &path);
    void ReadCalibrationFile(const std::string &path);
    void ReadEPICSChannels(const std::string &path);

    // dst data file
    void WriteToDST(const std::string &pat, std::ios::openmode mode = std::ios::out | std::ios::binary | std::ios::app);
    void WriteToDST(std::ofstream &dst_file, const EventData &data) throw(PRadException);
    void ReadFromDST(const std::string &path, std::ios::openmode mode = std::ios::in | std::ios::binary);
    void ReadFromDST(std::ifstream &dst_file, EventData &data) throw(PRadException);

    // evio data file
    void ReadFromEvio(const std::string &path);
    void Decode(const void *buffer);

    // data handler
    void Clear();
    void StartofNewEvent(const unsigned char &tag);
    void EndofThisEvent(const unsigned int &ev = 0);
    void FeedData(JLabTIData &tiData);
    void FeedData(ADC1881MData &adcData);
    void FeedData(GEMAPVData &gemData);
    void FeedData(TDCV767Data &tdcData);
    void FeedData(TDCV1190Data &tdcData);
    void FillTaggerHist(TDCV1190Data &tdcData);
    void UpdateEPICS(const std::string &name, const float &value);
    void UpdateTrgType(const unsigned char &trg);
    void UpdateScalarGroup(const unsigned int &size, const unsigned int *gated, const unsigned int *ungated);
    void AccumulateBeamCharge(const double &c);
 
    // show data
    int GetCurrentEventNb();
    void ChooseEvent(const int &idx = -1);
    unsigned int GetEventCount() {return energyData.size();};
    unsigned int GetEPICSEventCount() {return epicsData.size();};
    unsigned int GetScalarCount(const unsigned int &group = 0, const bool &gated = false);
    double GetBeamCharge() {return charge;};
    std::vector<unsigned int> GetScalarsCount(const bool &gated = false);
    TH1D *GetEnergyHist() {return energyHist;};
    TH2I *GetTagEHist() {return TagEHist;};
    TH2I *GetTagTHist() {return TagTHist;};
    EventData &GetEventData(const unsigned int &index);
    EventData &GetLastEvent();
    EPICSData &GetEPICSData(const unsigned int &index);
    double GetEnergy() {return totalE;};
    float GetEPICSValue(const std::string &name);
    float GetEPICSValue(const std::string &name, const int &index);
    void PrintOutEPICS();
    void PrintOutEPICS(const std::string &name);

    // analysis tools
    void InitializeByData(const std::string &path = "", int run = -1);
    void ResetChannelHists();
    void SaveHistograms(const std::string &path);
    void FitHistogram(const std::string &channel,
                      const std::string &hist_name,
                      const std::string &fit_func,
                      const double &range_min,
                      const double &range_max) throw(PRadException);
    void FitPedestal();
    void ReadGainFactor(const std::string &path, const int &ref = 2);
    void CorrectGainFactor(const int &run = 0, const int &ref = 2);
    void RefillEnergyHist();
    void RefillChannelHists();

    // other functions
    int GetRunNumberFromFileName(const std::string &name, const size_t &pos = 0, const bool &verbose = true);
    std::vector<epics_ch> GetSortedEPICSList();
    void SaveEPICSChannels(const std::string &path);

private:
    PRadEvioParser *parser;
    double totalE;
    double charge;
    bool onlineMode;
#ifdef MULTI_THREAD
    std::mutex myLock;
#endif
    std::unordered_map< ChannelAddress, PRadDAQUnit* > map_daq;
    std::unordered_map< std::string, PRadDAQUnit* > map_name;
    std::unordered_map< std::string, PRadTDCGroup* > map_name_tdc;
    std::unordered_map< ChannelAddress, PRadTDCGroup* > map_daq_tdc;
    std::unordered_map< std::string, size_t > epics_map;
    std::vector< float > epics_values;
    std::vector< PRadDAQUnit* > channelList;
    std::vector< PRadDAQUnit* > freeList; // channels that should be freed by handler
    std::vector< PRadTDCGroup* > tdcList;
    std::vector< ScalarChannel > triggerScalars;
    std::deque< EventData > energyData;
    std::deque< EPICSData > epicsData;
    EventData newEvent;
    TH1D *energyHist;
    TH2I *TagEHist;
    TH2I *TagTHist;
};

#endif
