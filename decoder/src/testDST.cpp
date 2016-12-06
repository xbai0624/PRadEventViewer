//============================================================================//
// An example showing how to use DST Parser to read and save selected events  //
//                                                                            //
// Chao Peng                                                                  //
// 11/20/2016                                                                 //
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

    PRadDSTParser *dst_parser = new PRadDSTParser(handler);
    //dst_parser->OpenInput("/work/hallb/prad/replay/prad_001288.dst");
    //dst_parser->OpenOutput("test.dst");
    dst_parser->OpenInput("prad_1310.dst");
    dst_parser->OpenOutput("prad_1310_select.dst");
    int count = 0;
    float beam_energy = 1097;

    // uncomment next line, it will not update calibration factor from dst file

    while(dst_parser->Read())
    {
        if(dst_parser->EventType() == PRad_DST_Event) {
            ++count;
            // you can push this event into data handler
            // handler->GetEventData().push_back(dst_parser->GetEvent()
            // or you can just do something with this event and discard it
            auto event = dst_parser->GetEvent();
            if(!event.is_physics_event())
                continue;

            hycal->Reconstruct(event);

            // only save the event with only 1 cluster and 1000+ MeV energy
            float energy = 0;
            for(auto &hit : hycal->GetDetector()->GetHits())
            {
                energy += hit.E;
            }

            if(energy >= beam_energy*0.7 && energy <= beam_energy*1.3)
                dst_parser->WriteEvent();

        } else if (dst_parser->EventType() == PRad_DST_Epics) {
            dst_parser->WriteEPICS();
        }
    }

    dst_parser->CloseInput();
    dst_parser->CloseOutput();
    return 0;
}

