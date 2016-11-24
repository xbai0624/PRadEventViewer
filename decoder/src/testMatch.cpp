//============================================================================//
// An example showing how to reconstruct, transform and match the detectors   //
//                                                                            //
// Chao Peng                                                                  //
// 10/24/2016                                                                 //
//============================================================================//

#include "PRadDataHandler.h"
#include "PRadDSTParser.h"
#include "PRadInfoCenter.h"
#include "PRadBenchMark.h"
#include "PRadEPICSystem.h"
#include "PRadTaggerSystem.h"
#include "PRadHyCalSystem.h"
#include "PRadGEMSystem.h"
#include "PRadCoordSystem.h"
#include "PRadDetMatch.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{

    PRadEPICSystem *epics = new PRadEPICSystem("config/epics_channels.conf");
    PRadHyCalSystem *hycal = new PRadHyCalSystem("config/hycal.conf");
    PRadGEMSystem *gem = new PRadGEMSystem("config/gem.conf");

    PRadDSTParser *dst_parser = new PRadDSTParser();

    // coordinate system and detector match system
    PRadCoordSystem *coord_sys = new PRadCoordSystem("database/coordinates.dat");
    PRadDetMatch *det_match = new PRadDetMatch("config/det_match.conf");

    PRadBenchMark timer;

    // here shows an example how to read DST file while not saving all the events
    // in memory
    dst_parser->OpenInput("/work/hallb/prad/replay/prad_001288.dst");

    PRadHyCalDetector *hycal_det = hycal->GetDetector();
    PRadGEMDetector *gem_det1 = gem->GetDetector("PRadGEM1");
    PRadGEMDetector *gem_det2 = gem->GetDetector("PRadGEM2");

    int count = 0;
    while(dst_parser->Read() && count < 50000)
    {
        if(dst_parser->EventType() == PRad_DST_Event) {
            ++count;
            // you can push this event into data handler
            // handler->GetEventData().push_back(dst_parser->GetEvent()
            // or you can just do something with this event and discard it
            auto event = dst_parser->GetEvent();

            // only interested in physics event
            if(!event.is_physics_event())
                continue;

            // update run information
            PRadInfoCenter::Instance().UpdateInfo(event);

            // reconstruct
            hycal->Reconstruct(event);
            gem->Reconstruct(event);

            // get reconstructed clusters
            auto hycal_hit = hycal->GetDetector()->GetHits();
            auto gem1_hit = gem->GetDetector("PRadGEM1")->GetHits();
            auto gem2_hit = gem->GetDetector("PRadGEM2")->GetHits();

            // coordinates transform, projection
            // you can either pass iterators
            coord_sys->Transform(hycal_det);
            coord_sys->Transform(gem_det1);
            coord_sys->Transform(gem_det2);
            // or arrays
            coord_sys->Projection(hycal_hit.begin(), hycal_hit.end());
            coord_sys->Projection(gem1_hit.begin(), gem1_hit.end());
            coord_sys->Projection(gem2_hit.begin(), gem2_hit.end());

            // hits matching, return matched index
            auto matched = det_match->Match(hycal_hit, gem1_hit, gem2_hit);

            cout << event.event_number << "  ";
            if(matched.empty())
                cout << "No matched event" << endl;

            for(auto idx : matched)
            {
                cout << "HyCal: " << hycal_hit[idx.hycal].x
                     << ", " << hycal_hit[idx.hycal].y
                     << ", " << hycal_hit[idx.hycal].E
                     << ". ";
                if(idx.gem1 >= 0) {
                    cout << "GEM 1: " << gem1_hit[idx.gem1].x
                         << ", " << gem1_hit[idx.gem1].y
                         << ". ";
                }
                if(idx.gem2 >= 0) {
                    cout << "GEM 2: " << gem2_hit[idx.gem2].x
                         << ", " << gem2_hit[idx.gem2].y
                         << ". ";
                }
                cout << endl;
            }

        } else if(dst_parser->EventType() == PRad_DST_Epics) {
            // save epics into handler, otherwise get epicsvalue won't work
            epics->AddEvent(dst_parser->GetEPICSEvent());
        }
    }

    dst_parser->CloseInput();

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << "Read " << count << " events and "
         << epics->GetEventCount() << " EPICS events from file."
         << endl;
    cout << PRadInfoCenter::GetBeamCharge() << endl;
    cout << PRadInfoCenter::GetLiveTime() << endl;

//    handler->WriteToDST("prad_001323_0-10.dst");
    //handler->PrintOutEPICS();
    return 0;
}
