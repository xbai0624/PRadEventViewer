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
class PRadEPICSystem;
class TH2I;

class PRadDataHandler
{
public:
    PRadDataHandler();
    virtual ~PRadDataHandler();

    // mode change
    void SetOnlineMode(const bool &mode);

    void SetHyCalSystem(PRadHyCalSystem *hycal) {hycal_sys = hycal;};
    void SetGEMSystem(PRadGEMSystem *gem) {gem_sys = gem;};
    PRadHyCalSystem *GetHyCalSystem() const {return hycal_sys;};
    PRadGEMSystem *GetGEMSystem() const {return gem_sys;};
    PRadEPICSystem *GetEPICSystem() const {return epic_sys;};

    // read config files
    void ReadConfig(const std::string &path);
    template<typename... Args>
    void ExecuteConfigCommand(void (PRadDataHandler::*act)(Args...), Args&&... args);

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
    void FeedData(EPICSRawData &epicsData);
    void FeedTaggerHits(TDCV1190Data &tdcData);
    void FillHistograms(EventData &data);
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
    int GetRunNumber() {return runInfo.run_number;};
    double GetBeamCharge() {return runInfo.beam_charge;};
    double GetLiveTime() {return (1. - runInfo.dead_count/runInfo.ungated_count);};
    TH2I *GetTagEHist() {return TagEHist;};
    TH2I *GetTagTHist() {return TagTHist;};
    EventData &GetEvent(const unsigned int &index) throw (PRadException);
    std::deque<EventData> &GetEventData() {return energyData;};
    RunInfo &GetRunInfo() {return runInfo;};
    OnlineInfo &GetOnlineInfo() {return onlineInfo;};
    double GetEnergy() {return totalE;};
    double GetEnergy(const EventData &event);

    // analysis tools
    void InitializeByData(const std::string &path = "", int run = -1, int ref = DEFAULT_REF_PMT);
    void RefillEnergyHist();
    int FindEventIndex(const int &event_number);


    // other functions
    void GetRunNumberFromFileName(const std::string &name, const size_t &pos = 0, const bool &verbose = true);

private:
    PRadEvioParser *parser;
    PRadDSTParser *dst_parser;
    PRadHyCalSystem *hycal_sys;
    PRadGEMSystem *gem_sys;
    PRadEPICSystem *epic_sys;
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

    EventData *newEvent;
    TH2I *TagEHist;
    TH2I *TagTHist;
};

#endif
