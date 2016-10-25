//============================================================================//
// An example showing how to reconstruct, transform and match the detectors   //
//                                                                            //
// Chao Peng                                                                  //
// 10/24/2016                                                                 //
//============================================================================//

#include "PRadDataHandler.h"
#include "PRadDSTParser.h"
#include "PRadBenchMark.h"
#include "PRadDAQUnit.h"
#include "PRadGEMSystem.h"
#include "PRadCoordSystem.h"
#include "PRadDetMatch.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{

    PRadDataHandler *handler = new PRadDataHandler();
    PRadDSTParser *dst_parser = new PRadDSTParser(handler);
    // read configuration files
    handler->ReadConfig("config.txt");

    // coordinate system and detector match system
    PRadCoordSystem *coord_sys = new PRadCoordSystem("config/coordinates.dat");
    PRadDetMatch *det_match = new PRadDetMatch("config/det_match.conf");

    PRadBenchMark timer;

    // here shows an example how to read DST file while not saving all the events
    // in memory
    dst_parser->OpenInput("/work/hallb/prad/replay/prad_001287.dst");

    int count = 0;

    // uncomment next line, it will not update calibration factor from dst file
    //dst_parser->SetMode(NO_HYCAL_CAL_UPDATE);
    PRadGEMSystem *gem_srs = handler->GetSRS();

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

            // reconstruct
            handler->HyCalReconstruct(event);
            gem_srs->Reconstruct(event);

            // get reconstructed clusters
            int n, n1, n2;
            HyCalHit *hycal_hit = handler->GetHyCalCluster(n);
            GEMHit *gem1_hit = gem_srs->GetDetector("PRadGEM1")->GetCluster(n1);
            GEMHit *gem2_hit = gem_srs->GetDetector("PRadGEM2")->GetCluster(n2);

            // coordinates transform, projection
            // you can either pass iterators
            coord_sys->Transform(&hycal_hit[0], &hycal_hit[n]);
            coord_sys->Transform(&gem1_hit[0], &gem1_hit[n1]);
            coord_sys->Transform(&gem2_hit[0], &gem2_hit[n2]);
            // or arrays
            coord_sys->Projection(hycal_hit, n);
            coord_sys->Projection(gem1_hit, n1);
            coord_sys->Projection(gem2_hit, n2);

            // hits matching, return matched index
            auto matched = det_match->Match(hycal_hit, n, gem1_hit, n1, gem2_hit, n2);

            cout << event.event_number << "  ";
            if(matched.empty())
                cout << "No matched event" << endl;

            for(auto idx : matched)
            {
                cout << "HyCal: " << hycal_hit[idx.hycal].x
                     << ", " << hycal_hit[idx.hycal].y
                     << ", " << hycal_hit[idx.hycal].E;
                if(idx.gem1 >= 0) {
                    cout << "GEM 1: " << gem1_hit[idx.gem1].x
                         << ", " << gem1_hit[idx.gem1].y;
                }
                if(idx.gem2 >= 0) {
                    cout << "GEM 2: " << gem2_hit[idx.gem2].x
                         << ", " << gem2_hit[idx.gem2].y;
                }
                cout << endl;
            }

        } else if(dst_parser->EventType() == PRad_DST_Epics) {
            // save epics into handler, otherwise get epicsvalue won't work
            handler->GetEPICSData().push_back(dst_parser->GetEPICSEvent());
        }
    }

    dst_parser->CloseInput();

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << "Read " << handler->GetEventCount() << " events and "
         << handler->GetEPICSEventCount() << " EPICS events from file."
         << endl;
    cout << handler->GetBeamCharge() << endl;
    cout << handler->GetLiveTime() << endl;

//    handler->WriteToDST("prad_001323_0-10.dst");
    //handler->PrintOutEPICS();
    return 0;
}
