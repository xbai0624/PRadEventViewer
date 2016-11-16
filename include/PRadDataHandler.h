#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

#include <unordered_map>
#include <map>
#include <deque>
#include <fstream>
#include "PRadEventStruct.h"
#include "PRadException.h"
#include "ConfigParser.h"
#include <thread>

// PMT 0 - 2
#define DEFAULT_REF_PMT 2

class PRadEvioParser;
class PRadDSTParser;
class PRadHyCalSystem;
class PRadGEMSystem;
class PRadGEMAPV;
class TH1D;
class TH2I;

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

class PRadDataHandler
{
public:
    PRadDataHandler();
    virtual ~PRadDataHandler();

    // mode change
    void SetOnlineMode(const bool &mode);

    // add channels
    void RegisterEPICS(const std::string &name, const uint32_t &id, const float &value);

    PRadHyCalSystem *GetHyCalSystem() {return hycal_sys;};
    PRadGEMSystem *GetGEMSystem() {return gem_sys;};

    // read config files
    void ReadConfig(const std::string &path);
    template<typename... Args>
    void ExecuteConfigCommand(void (PRadDataHandler::*act)(Args...), Args&&... args);
    void ReadEPICSChannels(const std::string &path);

    // file reading and writing
    void ReadFromDST(const std::string &path, const uint32_t &mode = 0);
    void ReadFromEvio(const std::string &path, const int &evt = -1, const bool &verbose = false);
    void ReadFromSplitEvio(const std::string &path, const int &split = -1, const bool &verbose = true);
    void Decode(const void *buffer);
    void Replay(const std::string &r_path, const int &split = -1, const std::string &w_path = "");
    void WriteToDST(const std::string &path);

    // data handler
    void Clear();
    void SetRunNumber(const int &run) {runInfo.run_number = run;};
    void StartofNewEvent(const unsigned char &tag);
    void EndofThisEvent(const unsigned int &ev);
    void EndProcess(EventData *data);
    void WaitEventProcess();
    void FeedData(JLabTIData &tiData);
    void FeedData(JLabDSCData &dscData);
    void FeedData(ADC1881MData &adcData);
    void FeedData(TDCV767Data &tdcData);
    void FeedData(TDCV1190Data &tdcData);
    void FeedData(GEMRawData &gemData);
    void FeedData(std::vector<GEMZeroSupData> &gemData);
    void FeedTaggerHits(TDCV1190Data &tdcData);
    void FillHistograms(EventData &data);
    void UpdateEPICS(const std::string &name, const float &value);
    void UpdateTrgType(const unsigned char &trg);
    void AccumulateBeamCharge(EventData &event);
    void UpdateLiveTimeScaler(EventData &event);
    void UpdateOnlineInfo(EventData &event);
    void UpdateRunInfo(const RunInfo &ri) {runInfo = ri;};

    // show data
    int GetCurrentEventNb();
    void ChooseEvent(const int &idx = -1);
    void ChooseEvent(const EventData &event);
    unsigned int GetEventCount() {return energyData.size();};
    unsigned int GetEPICSEventCount() {return epicsData.size();};
    int GetRunNumber() {return runInfo.run_number;};
    double GetBeamCharge() {return runInfo.beam_charge;};
    double GetLiveTime() {return (1. - runInfo.dead_count/runInfo.ungated_count);};
    TH2I *GetTagEHist() {return TagEHist;};
    TH2I *GetTagTHist() {return TagTHist;};
    EventData &GetEvent(const unsigned int &index);
    EventData &GetLastEvent();
    std::deque<EventData> &GetEventData() {return energyData;};
    EPICSData &GetEPICSEvent(const unsigned int &index);
    std::deque<EPICSData> &GetEPICSData() {return epicsData;};
    RunInfo &GetRunInfo() {return runInfo;};
    OnlineInfo &GetOnlineInfo() {return onlineInfo;};
    double GetEnergy() {return totalE;};
    double GetEnergy(const EventData &event);
    float GetEPICSValue(const std::string &name);
    float GetEPICSValue(const std::string &name, const int &index);
    float GetEPICSValue(const std::string &name, const EventData &event);
    void PrintOutEPICS();
    void PrintOutEPICS(const std::string &name);

    // analysis tools
    void InitializeByData(const std::string &path = "", int run = -1, int ref = DEFAULT_REF_PMT);
    void RefillEnergyHist();
    int FindEventIndex(const int &event_number);


    // other functions
    void GetRunNumberFromFileName(const std::string &name, const size_t &pos = 0, const bool &verbose = true);
    std::vector<epics_ch> GetSortedEPICSList();
    void SaveEPICSChannels(const std::string &path);

private:
    PRadEvioParser *parser;
    PRadDSTParser *dst_parser;
    PRadHyCalSystem *hycal_sys;
    PRadGEMSystem *gem_sys;
    RunInfo runInfo;
    OnlineInfo onlineInfo;
    double totalE;
    bool onlineMode;
    bool replayMode;
    int current_event;
    std::thread end_thread;

    // data related
    std::unordered_map< std::string, uint32_t > epics_map;
    std::vector< float > epics_values;
    std::deque< EventData > energyData;
    std::deque< EPICSData > epicsData;

    EventData *newEvent;
    TH2I *TagEHist;
    TH2I *TagTHist;
};

#endif
