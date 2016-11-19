#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

#include <deque>
#include <thread>
#include <unordered_map>
#include "PRadEvioParser.h"
#include "PRadDSTParser.h"
#include "PRadEventStruct.h"
#include "PRadException.h"

// PMT 0 - 2
#define DEFAULT_REF_PMT 2

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

    // set systems
    void SetHyCalSystem(PRadHyCalSystem *hycal) {hycal_sys = hycal;};
    void SetGEMSystem(PRadGEMSystem *gem) {gem_sys = gem;};
    void SetEPICSystem(PRadEPICSystem *epics) {epic_sys = epics;};
    PRadHyCalSystem *GetHyCalSystem() const {return hycal_sys;};
    PRadGEMSystem *GetGEMSystem() const {return gem_sys;};
    PRadEPICSystem *GetEPICSystem() const {return epic_sys;};

    // file reading and writing
    void Decode(const void *buffer);
    void ReadFromDST(const std::string &path, const uint32_t &mode = 0);
    void ReadFromEvio(const std::string &path, const int &evt = -1, const bool &verbose = false);
    void ReadFromSplitEvio(const std::string &path, const int &split = -1, const bool &verbose = true);
    void WriteToDST(const std::string &path);
    void Replay(const std::string &r_path, const int &split = -1, const std::string &w_path = "");

    // data handler
    void Clear();
    void StartofNewEvent(const unsigned char &tag);
    void EndofThisEvent(const unsigned int &ev);
    void EndProcess(EventData *data);
    void FillHistograms(EventData &data);
    void UpdateTrgType(const unsigned char &trg);


    // feeding data
    void FeedData(JLabTIData &tiData);
    void FeedData(JLabDSCData &dscData);
    void FeedData(ADC1881MData &adcData);
    void FeedData(TDCV767Data &tdcData);
    void FeedData(TDCV1190Data &tdcData);
    void FeedData(GEMRawData &gemData);
    void FeedData(std::vector<GEMZeroSupData> &gemData);
    void FeedData(EPICSRawData &epicsData);
    void FeedTaggerHits(TDCV1190Data &tdcData);


    // show data
    void ChooseEvent(const int &idx = -1);
    void ChooseEvent(const EventData &event);
    int GetCurrentEventNb() const {return current_event;};
    unsigned int GetEventCount() const {return event_data.size();};
    TH2I *GetTagEHist() const {return TagEHist;};
    TH2I *GetTagTHist() const {return TagTHist;};
    const EventData &GetEvent(const unsigned int &index) const throw (PRadException);
    const std::deque<EventData> &GetEventData() const {return event_data;};

    // analysis tools
    void InitializeByData(const std::string &path = "", int ref = DEFAULT_REF_PMT);
    void RefillEnergyHist();
    int FindEvent(int event_number) const;


    // other functions
    void GetRunNumberFromFileName(const std::string &name, const size_t &pos = 0, const bool &verbose = true);

private:
    void waitEventProcess();

private:
    PRadEvioParser parser;
    PRadDSTParser dst_parser;
    PRadHyCalSystem *hycal_sys;
    PRadGEMSystem *gem_sys;
    PRadEPICSystem *epic_sys;
    bool onlineMode;
    bool replayMode;
    int current_event;
    std::thread end_thread;

    // data related
    std::deque<EventData> event_data;

    EventData *new_event;
    TH2I *TagEHist;
    TH2I *TagTHist;
};

#endif
