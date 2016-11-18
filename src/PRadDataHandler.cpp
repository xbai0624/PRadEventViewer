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
#include "PRadHyCalSystem.h"
#include "PRadGEMSystem.h"
#include "PRadEPICSystem.h"
#include "PRadBenchMark.h"
#include "ConfigParser.h"
#include "canalib.h"
#include "TH2.h"

#define EPICS_UNDEFINED_VALUE -9999.9

using namespace std;

// constructor
PRadDataHandler::PRadDataHandler()
: parser(this), dst_parser(this),
  hycal_sys(nullptr), gem_sys(nullptr), onlineMode(false), replayMode(false), current_event(0)
{
    TagEHist = new TH2I("Tagger E", "Tagger E counter", 2000, 0, 20000, 384, 0, 383);
    TagTHist = new TH2I("Tagger T", "Tagger T counter", 2000, 0, 20000, 128, 0, 127);

    onlineInfo.add_trigger("Lead Glass Sum", 0);
    onlineInfo.add_trigger("Total Sum", 1);
    onlineInfo.add_trigger("LMS Led", 2);
    onlineInfo.add_trigger("LMS Alpha Source", 3);
    onlineInfo.add_trigger("Tagger Master OR", 4);
    onlineInfo.add_trigger("Scintillator", 5);
}

// destructor
PRadDataHandler::~PRadDataHandler()
{
    delete TagEHist;
    delete TagTHist;
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
    runInfo.clear();

    TagEHist->Reset();
    TagTHist->Reset();

    if(epic_sys)
        epic_sys->Reset();
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

void PRadDataHandler::AccumulateBeamCharge(EventData &event)
{
    if(event.is_physics_event())
        runInfo.beam_charge += event.get_beam_charge();
}

void PRadDataHandler::UpdateLiveTimeScaler(EventData &event)
{
    if(event.is_physics_event()) {
        runInfo.ungated_count += event.get_ref_channel().ungated_count;
        runInfo.dead_count += event.get_ref_channel().gated_count;
    }
}

void PRadDataHandler::UpdateOnlineInfo(EventData &event)
{
    // update triggers
    for(auto trg_ch : onlineInfo.trigger_info)
    {
        if(trg_ch.id < event.dsc_data.size())
        {
            // get ungated trigger counts
            unsigned int counts = event.get_dsc_channel(trg_ch.id).ungated_count;

            // calculate the frequency
            trg_ch.freq = (double)counts / event.get_beam_time();
        }

        else {
            cerr << "Data Handler: Unmatched discriminator data from event "
                 << event.event_number
                 << ", expect trigger " << trg_ch.name
                 << " at channel " << trg_ch.id
                 << ", but the event only has " << event.dsc_data.size()
                 << " dsc channels." << endl;
        }
    }

    // update live time
    onlineInfo.live_time = event.get_live_time();

    //update beam current
    onlineInfo.beam_current = event.get_beam_current();
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

    if(tdcData.addr.crate != PRadTS) {
        FeedTaggerHits(tdcData);
        return;
    }

    PRadTDCChannel *tdc = hycal_sys->GetTDCChannel(tdcData.addr);

    if(!tdc)
        return;

    new_event->add_tdc(TDC_Data(tdc->GetID(), tdcData.val));
}

void PRadDataHandler::FeedTaggerHits(TDCV1190Data &tdcData)
{
#define TAGGER_CHANID 30000 // Tagger tdc id will start from this number
#define TAGGER_T_CHANID 1000 // Start from TAGGER_CHANID, more than 1000 will be t channel
    // E channel
    if(tdcData.addr.slot == 3 || tdcData.addr.slot == 5 || tdcData.addr.slot == 7)
    {
        int e_ch = tdcData.addr.channel + (tdcData.addr.slot - 3)*64 + TAGGER_CHANID;
        // E Channel 30000 + channel
        new_event->add_tdc(TDC_Data(e_ch, tdcData.val));
    }
    // T Channel
    if(tdcData.addr.slot == 14)
    {
        int t_lr = tdcData.addr.channel/64;
        int t_ch = tdcData.addr.channel%64;
        if(t_ch > 31)
            t_ch = 32 + (t_ch + 16)%32;
        else
            t_ch = (t_ch + 16)%32;
        t_ch += t_lr*64;
        new_event->add_tdc(TDC_Data(t_ch + TAGGER_CHANID + TAGGER_T_CHANID, tdcData.val));
    }
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
    if(!hycal_sys)
        return;

    double energy = 0.;

    // for all types of events
    for(auto &adc : data.adc_data)
    {
        PRadADCChannel *channel = hycal_sys->GetADCChannel(adc.channel_id);
        if(!channel)
            continue;

        channel->FillHist(adc.value, data.trigger);
        energy += channel->GetEnergy(adc.value);
    }

    if(!data.is_physics_event())
        return;

    // for only physics events
    hycal_sys->FillEnergyHist(energy);

    for(auto &tdc : data.tdc_data)
    {
        PRadTDCChannel *channel = hycal_sys->GetTDCChannel(tdc.channel_id);
        if(channel) {
            channel->FillHist(tdc.value);
        } else if(tdc.channel_id >= TAGGER_CHANID) {
            int id = tdc.channel_id - TAGGER_CHANID;
            if(id >= TAGGER_T_CHANID)
                TagTHist->Fill(tdc.value, id - TAGGER_T_CHANID);
            else
                TagEHist->Fill(tdc.value, id - TAGGER_CHANID);
        }
    }
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
                dst_parser.WriteEPICS(EPICSData(data->event_number, epic_sys->GetValues()));
            else
                epic_sys->SaveData(data->event_number, onlineMode);
        }
    } else { // event or sync event

        FillHistograms(*data);

        if(data->type == CODA_Sync) {
            AccumulateBeamCharge(*data);
            UpdateLiveTimeScaler(*data);
            if(onlineMode)
                UpdateOnlineInfo(*data);
        }

        if(onlineMode && event_data.size()) // online mode only saves the last event, to reduce usage of memory
            event_data.pop_front();

        if(replayMode)
            dst_parser.WriteEvent(*data);
        else
            event_data.emplace_back(move(*data)); // save event

    }

    delete data; // new data memory is released here
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

void PRadDataHandler::InitializeByData(const string &path, int run, int ref)
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
        // auto update run number
        if(run < 0)
            GetRunNumberFromFileName(path);
        else
            SetRunNumber(run);

        gem_sys->SetPedestalMode(true);

        parser.ReadEvioFile(path.c_str(), 20000);
    }

    cout << "Data Handler: Fitting Pedestal for HyCal." << endl;
    hycal_sys->FitPedestal();

    cout << "Data Handler: Correct HyCal Gain Factor, Run Number: " << runInfo.run_number << "." << endl;
    hycal_sys->CorrectGainFactor(ref);

    cout << "Data Handler: Fitting Pedestal for GEM." << endl;
    gem_sys->FitPedestal();
//    gem_sys->SavePedestal("gem_ped_" + to_string(runInfo.run_number) + ".dat");
//    gem_sys->SaveHistograms("gem_ped_" + to_string(runInfo.run_number) + ".root");

    cout << "Data Handler: Releasing Memeory." << endl;
    gem_sys->SetPedestalMode(false);

    // save run number
    int run_number = runInfo.run_number;
    Clear();
    SetRunNumber(run_number);

    cout << "Data Handler: Done initialization, took " << timer.GetElapsedTime()/1000. << " s" << endl;
}

void PRadDataHandler::GetRunNumberFromFileName(const string &name, const size_t &pos, const bool &verbose)
{
    // get rid of suffix
    auto nameEnd = name.find(".evio");

    if(nameEnd == string::npos)
        nameEnd = name.size();
    else
        nameEnd -= 1;

    // get rid of directories
    auto nameBeg = name.find_last_of("/");
    if(nameBeg == string::npos)
        nameBeg = 0;
    else
        nameBeg += 1;

    int number = ConfigParser::find_integer(name.substr(nameBeg, nameEnd - nameBeg + 1), pos);

    if(number > 0) {

        if(verbose) {
            cout << "Data Handler: Run number is automatcially determined from file name."
                 << endl
                 << "File name: " << name
                 << endl
                 << "Run number: " << number
                 << endl;
        }

        SetRunNumber(number);
    }
}

// find event by its event number
// it is assumed the files decoded are all from 1 single run and they are loaded in order
// otherwise this function will not work properly
int PRadDataHandler::FindEvent(int evt)
const
{
    auto event_cmp = [] (const EventData &event, const int &evt)
                     {
                        return event.event_number - evt;
                     };
    auto it = cana::binary_search(event_data.begin(), event_data.end(), evt, event_cmp);

    if(it == event_data.end())
        return -1;

    return it - event_data.begin();
}

void PRadDataHandler::Replay(const string &r_path, const int &split, const string &w_path)
{
    if(w_path.empty()) {
        string file = "prad_" + to_string(runInfo.run_number) + ".dst";
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

    dst_parser.WriteRunInfo(this);

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

        dst_parser.WriteRunInfo(this);

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
