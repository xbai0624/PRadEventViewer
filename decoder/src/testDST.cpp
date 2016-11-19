//============================================================================//
// An application of replay raw data file and save the replayed data into DST //
// file. This is the 1st-level replay, it only discards the pedestal data     //
//                                                                            //
// Chao Peng                                                                  //
// 10/04/2016                                                                 //
//============================================================================//

#include "PRadDataHandler.h"
#include "PRadEPICSystem.h"
#include "PRadTaggerSystem.h"
#include "PRadHyCalSystem.h"
#include "PRadGEMSystem.h"
#include "PRadInfoCenter.h"
#include "PRadEvioParser.h"
#include "PRadBenchMark.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main()
{
    PRadDataHandler *handler = new PRadDataHandler();
    PRadEPICSystem *epics = new PRadEPICSystem("config/epics_channels.conf");
    PRadHyCalSystem *hycal = new PRadHyCalSystem("config/hycal.conf");
    PRadGEMSystem *gem = new PRadGEMSystem("config/gem.conf");
    PRadTaggerSystem *tagger = new PRadTaggerSystem;

    handler->SetEPICSystem(epics);
    handler->SetTaggerSystem(tagger);
    handler->SetHyCalSystem(hycal);
    handler->SetGEMSystem(gem);

    PRadBenchMark timer;
    handler->ReadFromDST("test.dst");

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << "Read " << handler->GetEventCount() << " events and "
         << epics->GetEventCount() << " EPICS events from file."
         << endl;
    cout << PRadInfoCenter::GetBeamCharge() << endl;
    cout << PRadInfoCenter::GetLiveTime() << endl;

    handler->WriteToDST("test.dst");
    return 0;
}

