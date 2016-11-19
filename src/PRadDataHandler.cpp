//============================================================================//
// The data handler and container class                                       //
// Dealing with the data from all the channels                                //
//                                                                            //
// Chao Peng                                                                  //
// 02/07/2016                                                                 //
//============================================================================//

#include <iostream>
#include <iomanip>
#include <algorithm>
#include "PRadDataHandler.h"
#include "PRadInfoCenter.h"
#include "PRadEPICSystem.h"
#include "PRadTaggerSystem.h"
#include "PRadHyCalSystem.h"
#include "PRadGEMSystem.h"
#include "PRadBenchMark.h"
#include "ConfigParser.h"
#include "canalib.h"
#include "TH2.h"

#define EPICS_UNDEFINED_VALUE -9999.9

using namespace std;

// constructor
PRadDataHandler::PRadDataHandler()
: parser(this), dst_parser(this),
  epic_sys(nullptr), tagger_sys(nullptr), hycal_sys(nullptr), gem_sys(nullptr),
  onlineMode(false), replayMode(false), current_event(0)
{
    // place holder
}

// destructor
PRadDataHandler::~PRadDataHandler()
{
    // place holder
}

// decode an event buffer
void PRadDataHandler::Decode(const void *buffer)
{
    parser.ReadEventBuffer(buffer);

    waitEventProcess();
}

void PRadDataHandler::ReadFromDST(const string &path, const uint32_t &mode)
{
    try {
        dst_parser.OpenInput(path);

        dst_parser.SetMode(mode);

        cout << "Data Handler: Reading events from DST file "
             << "\"" << path << "\""
             << endl;

        while(dst_parser.Read())
        {
            switch(dst_parser.EventType())
            {
            case PRad_DST_Event:
                FillHistograms(dst_parser.GetEvent());
                event_data.push_back(dst_parser.GetEvent());
                break;
            case PRad_DST_Epics:
                if(epic_sys)
                    epic_sys->AddEvent(dst_parser.GetEPICSEvent());
                break;
            default:
                break;
            }
        }

    } catch(PRadException &e) {
        cerr << e.FailureType() << ": "
             << e.FailureDesc() << endl
             << "Write to DST Aborted!" << endl;
    } catch(exception &e) {
        cerr << e.what() << endl
             << "Write to DST Aborted!" << endl;
    }
    dst_parser.CloseInput();
 }


// read fro evio file
void PRadDataHandler::ReadFromEvio(const string &path, const int &evt, const bool &verbose)
{
    parser.ReadEvioFile(path.c_str(), evt, verbose);
    waitEventProcess();
}

//read from several evio file
void PRadDataHandler::ReadFromSplitEvio(const string &path, const int &split, const bool &verbose)
{
    if(split < 0) {// default input, no split
        ReadFromEvio(path.c_str(), -1, verbose);
    } else {
        for(int i = 0; i <= split; ++i)
        {
            string split_path = path + "." + to_string(i);
            ReadFromEvio(split_path.c_str(), -1, verbose);
        }
    }
}

// erase the data container
void PRadDataHandler::Clear()
{
    // used memory won't be released, but it can be used again for new data file
    event_data = deque<EventData>();
    parser.SetEventNumber(0);

    PRadInfoCenter::Instance().Reset();

    if(epic_sys)
        epic_sys->Reset();

    if(tagger_sys)
        tagger_sys->Reset();

    if(hycal_sys)
        hycal_sys->Reset();

    if(gem_sys)
        gem_sys->Reset();
}

void PRadDataHandler::UpdateTrgType(const unsigned char &trg)
{
    if(new_event->trigger && (new_event->trigger != trg)) {
        cerr << "ERROR: Trigger type mismatch at event "
             << parser.GetEventNumber()
             << ", was " << (int) new_event->trigger
             << " now " << (int) trg
             << endl;
    }
    new_event->trigger = trg;
}

void PRadDataHandler::FeedData(JLabTIData &tiData)
{
    new_event->timestamp = tiData.time_high;
    new_event->timestamp <<= 32;
    new_event->timestamp |= tiData.time_low;
}

void PRadDataHandler::FeedData(JLabDSCData &dscData)
{
    for(uint32_t i = 0; i < dscData.size; ++i)
    {
        new_event->dsc_data.emplace_back(dscData.gated_buf[i], dscData.ungated_buf[i]);
    }
}

// feed ADC1881M data
void PRadDataHandler::FeedData(ADC1881MData &adcData)
{
    if(!hycal_sys)
        return;

    // get the channel
    PRadADCChannel *channel = hycal_sys->GetADCChannel(adcData.addr);

    if(!channel)
        return;

    if(new_event->is_physics_event()) {
        if(channel->Sparsify(adcData.val)) {
            new_event->add_adc(ADC_Data(channel->GetID(), adcData.val)); // store this data word
        }
    } else if (new_event->is_monitor_event()) {
        new_event->add_adc(ADC_Data(channel->GetID(), adcData.val));
    }

}

void PRadDataHandler::FeedData(TDCV767Data &tdcData)
{
    if(!hycal_sys)
        return;

    PRadTDCChannel *tdc = hycal_sys->GetTDCChannel(tdcData.addr);

    if(!tdc)
        return;

    new_event->tdc_data.push_back(TDC_Data(tdc->GetID(), tdcData.val));
}

void PRadDataHandler::FeedData(TDCV1190Data &tdcData)
{
    if(!hycal_sys)
        return;

    // tagger hits
    if(tdcData.addr.crate == PRadTagE) {
        if(tagger_sys)
            tagger_sys->FeedTaggerHits(tdcData, *new_event);
        return;
    }

    PRadTDCChannel *tdc = hycal_sys->GetTDCChannel(tdcData.addr);

    if(!tdc)
        return;

    new_event->add_tdc(TDC_Data(tdc->GetID(), tdcData.val));
}

// feed GEM data
void PRadDataHandler::FeedData(GEMRawData &gemData)
{
    if(gem_sys)
        gem_sys->FillRawData(gemData, new_event->gem_data, new_event->is_monitor_event());
}

// feed GEM data which has been zero-suppressed
void PRadDataHandler::FeedData(vector<GEMZeroSupData> &gemData)
{
    if(gem_sys)
        gem_sys->FillZeroSupData(gemData, new_event->gem_data);
}

void PRadDataHandler::FeedData(EPICSRawData &epicsData)
{
    if(epic_sys)
        epic_sys->FillRawData(epicsData.buf);
}

//TODO move to hycal system
void PRadDataHandler::FillHistograms(EventData &data)
{
    if(hycal_sys)
        hycal_sys->FillHists(data);

    if(tagger_sys)
        tagger_sys->FillHists(data);
}

// signal of new event
void PRadDataHandler::StartofNewEvent(const unsigned char &tag)
{
    new_event = new EventData(tag);
}

// signal of event end, save event or discard event in online mode
void PRadDataHandler::EndofThisEvent(const unsigned int &ev)
{
    new_event->event_number = ev;
    // wait for the process thread
    waitEventProcess();

    end_thread = thread(&PRadDataHandler::EndProcess, this, new_event);
}

void PRadDataHandler::waitEventProcess()
{
    if(end_thread.joinable())
        end_thread.join();
}

void PRadDataHandler::EndProcess(EventData *data)
{
    if(data->type == EPICS_Info) {
        if(epic_sys) {
            if(replayMode)
                dst_parser.WriteEPICS(EPICS_Data(data->event_number, epic_sys->GetValues()));
            else
                epic_sys->SaveData(data->event_number, onlineMode);
        }
    } else { // event or sync event

        FillHistograms(*data);
        PRadInfoCenter::Instance().UpdateInfo(*data);

        if(onlineMode && event_data.size()) // online mode only saves the last event, to reduce usage of memory
            event_data.pop_front();

        if(replayMode)
            dst_parser.WriteEvent(*data);
        else
            event_data.emplace_back(move(*data)); // save event

    }

    delete data, data = nullptr; // new data memory is released here
}

// show the event to event viewer
void PRadDataHandler::ChooseEvent(const int &idx)
{
    if (event_data.size()) { // offline mode, pick the event given by console
        if((unsigned int) idx >= event_data.size())
            ChooseEvent(event_data.back());
        else
            ChooseEvent(event_data.at(idx));
    }
}

void PRadDataHandler::ChooseEvent(const EventData &event)
{
    if(hycal_sys)
        hycal_sys->ChooseEvent(event);
    if(gem_sys)
        gem_sys->ChooseEvent(event);

    current_event = event.event_number;
}

const EventData &PRadDataHandler::GetEvent(const unsigned int &index)
const
throw (PRadException)
{
    if(!event_data.size())
        throw PRadException("PRad Data Handler Error", "Empty data bank!");

    if(index >= event_data.size()) {
        return event_data.back();
    } else {
        return event_data.at(index);
    }
}

// Refill energy hist after correct gain factos
void PRadDataHandler::RefillEnergyHist()
{
    if(!hycal_sys)
        return;

    hycal_sys->ResetEnergyHist();

    for(auto &event : event_data)
    {
        if(!event.is_physics_event())
            continue;

        hycal_sys->FillEnergyHist(event);
    }
}

void PRadDataHandler::InitializeByData(const string &path, int ref)
{
    PRadBenchMark timer;

    if(!hycal_sys || !gem_sys) {
        cout << "Data Handler: HyCal System or GEM System missing, abort initialization."
             << endl;
        return;
    }

    cout << "Data Handler: Initializing from Data File "
         << "\"" << path << "\"."
         << endl;

    if(!path.empty()) {
        PRadInfoCenter::SetRunNumber(path);
        gem_sys->SetPedestalMode(true);
        parser.ReadEvioFile(path.c_str(), 20000);
    }

    cout << "Data Handler: Fitting Pedestal for HyCal." << endl;
    hycal_sys->FitPedestal();

    cout << "Data Handler: Correct HyCal Gain Factor. " << endl;
    hycal_sys->CorrectGainFactor(ref);

    cout << "Data Handler: Fitting Pedestal for GEM." << endl;
    gem_sys->FitPedestal();

    cout << "Data Handler: Releasing Memeory." << endl;
    gem_sys->SetPedestalMode(false);

    // save run number
    int run_number = PRadInfoCenter::GetRunNumber();
    Clear();
    PRadInfoCenter::SetRunNumber(run_number);

    cout << "Data Handler: Done initialization, took "
         << timer.GetElapsedTime()/1000. << " s"
         << endl;
}

// find event by its event number
// it is assumed the files decoded are all from 1 single run and they are loaded in order
// otherwise this function will not work properly
int PRadDataHandler::FindEvent(int evt)
const
{
    auto it = cana::binary_search(event_data.begin(), event_data.end(), evt);

    if(it == event_data.end())
        return -1;

    return it - event_data.begin();
}

void PRadDataHandler::Replay(const string &r_path, const int &split, const string &w_path)
{
    if(w_path.empty()) {
        string file = "prad_" + to_string(PRadInfoCenter::GetRunNumber()) + ".dst";
        dst_parser.OpenOutput(file);
    } else {
        dst_parser.OpenOutput(w_path);
    }

    cout << "Replay started!" << endl;
    PRadBenchMark timer;

    dst_parser.WriteHyCalInfo(hycal_sys);
    dst_parser.WriteGEMInfo(gem_sys);
    dst_parser.WriteEPICSMap(epic_sys);

    replayMode = true;

    ReadFromSplitEvio(r_path, split);

    dst_parser.WriteRunInfo();

    replayMode = false;

    cout << "Replay done, took " << timer.GetElapsedTime()/1000. << " s!" << endl;
    dst_parser.CloseOutput();
}

void PRadDataHandler::WriteToDST(const string &path)
{
    try {
        dst_parser.OpenOutput(path);

        cout << "Data Handler: Saving DST file "
             << "\"" << path << "\""
             << endl;

        dst_parser.WriteHyCalInfo(hycal_sys);
        dst_parser.WriteGEMInfo(gem_sys);
        dst_parser.WriteEPICSMap(epic_sys);

        if(epic_sys) {
            for(auto &epics : epic_sys->GetEventData())
            {
                dst_parser.WriteEPICS(epics);
            }
        }

        for(auto &event : event_data)
        {
            dst_parser.WriteEvent(event);
        }

        dst_parser.WriteRunInfo();

    } catch(PRadException &e) {
        cerr << e.FailureType() << ": "
             << e.FailureDesc() << endl
             << "Write to DST Aborted!" << endl;
    } catch(exception &e) {
        cerr << e.what() << endl
             << "Write to DST Aborted!" << endl;
    }

    dst_parser.CloseOutput();
}
