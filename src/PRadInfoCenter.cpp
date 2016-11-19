//============================================================================//
// A singleton class that stores and manages the run information or online    //
// information, it can be shared through all the classes                      //
//                                                                            //
// Chao Peng                                                                  //
// 11/19/2016                                                                 //
//============================================================================//

#include "PRadInfoCenter.h"
#include "ConfigParser.h"



// add the trigger channels
PRadInfoCenter::PRadInfoCenter()
{
    online_info.add_trigger("Lead Glass Sum", 0);
    online_info.add_trigger("Total Sum", 1);
    online_info.add_trigger("LMS Led", 2);
    online_info.add_trigger("LMS Alpha Source", 3);
    online_info.add_trigger("Tagger Master OR", 4);
    online_info.add_trigger("Scintillator", 5);
}

// clear all the information
void PRadInfoCenter::Reset()
{
    run_info.reset();
    online_info.reset();
}

// update information from event data
void PRadInfoCenter::UpdateInfo(const EventData &event)
{
    // only synchronization event contains the following information
    // this is by the design of our DAQ system
    if(event.get_type() != CODA_Sync)
        return;

    // online information update, update to the latest values
    // update triggers
    for(auto trg_ch : online_info.trigger_info)
    {
        if(trg_ch.id < event.dsc_data.size())
        {
            // get ungated trigger counts
            unsigned int counts = event.get_dsc_channel(trg_ch.id).ungated_count;

            // calculate the frequency
            trg_ch.freq = (double)counts / event.get_beam_time();
        } else {
            std::cerr << "PRad Info Center Erro: Unmatched discriminator data"
                      << " from event " << event.event_number
                      << ", expect trigger " << trg_ch.name
                      << " at channel " << trg_ch.id
                      << ", but the event only has " << event.dsc_data.size()
                      << " dsc channels."
                      << std::endl;
        }
    }

    // update live time
    online_info.live_time = event.get_live_time();

    //update beam current
    online_info.beam_current = event.get_beam_current();

    // run information, accu
    // only collect run information for physics events
    if(!event.is_physics_event())
        return;

    run_info.beam_charge += event.get_beam_charge();
    run_info.ungated_count += event.get_ref_channel().ungated_count;
    run_info.dead_count += event.get_ref_channel().gated_count;
}

// set run number
void PRadInfoCenter::SetRunNumber(int run)
{
    Instance().run_info.run_number = run;
}

// get run number
int PRadInfoCenter::GetRunNumber()
{
    return Instance().run_info.run_number;
}

// get beam charge
double PRadInfoCenter::GetBeamCharge()
{
    return Instance().run_info.beam_charge;
}

// get live time
double PRadInfoCenter::GetLiveTime()
{
    const RunInfo &run = Instance().run_info;
    if(!run.ungated_count)
        return 0.;
    return (1. - run.dead_count/run.ungated_count);
};

// try to determine run number from file
int __run_number_from_path(const std::string &name)
{
    // get rid of suffix
    auto nameEnd = name.find(".evio");

    if(nameEnd == std::string::npos)
        nameEnd = name.size();
    else
        nameEnd -= 1;

    // get rid of directories
    auto nameBeg = name.find_last_of("/");
    if(nameBeg == std::string::npos)
        nameBeg = 0;
    else
        nameBeg += 1;

    return ConfigParser::find_integer(name.substr(nameBeg, nameEnd - nameBeg + 1), 0);
}

// set run number from file path
void PRadInfoCenter::SetRunNumber(const std::string &path)
{
    int run = __run_number_from_path(path);
    if(run > 0)
        Instance().run_info.run_number = run;
}
