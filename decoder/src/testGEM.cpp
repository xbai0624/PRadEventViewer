//============================================================================//
// An example showing how to use GEM clustering method to reconstruct gem data//
//                                                                            //
// Chao Peng                                                                  //
// 10/07/2016                                                                 //
//============================================================================//

#include "PRadDataHandler.h"
#include "PRadDSTParser.h"
#include "PRadEvioParser.h"
#include "PRadBenchMark.h"
#include "PRadDAQUnit.h"
#include "PRadGEMSystem.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{

    PRadDataHandler *handler = new PRadDataHandler();
    PRadGEMSystem *gem_srs = new PRadGEMSystem("config/gem.conf");
    handler->SetGEMSystem(gem_srs);
    PRadDSTParser *dst_parser = new PRadDSTParser(handler);

    // read configuration files
    handler->ReadConfig("config.txt");

    PRadBenchMark timer;

    /* some test on connections constructors and assignment operators
    gem_srs->GetAPV(1, 5)->UnsetFEC();
    gem_srs->GetFEC(1)->RemoveAPV(6);

    PRadGEMSystem gem(*gem_srs);
    PRadGEMSystem gem2(move(gem));
    delete gem_srs;
    PRadGEMSystem gem3 = move(gem2);
    gem_srs = &gem3;
    */

    /* save gem pedestal
    gem_srs->SetPedestalMode(true);
    handler->InitializeByData("prad_001287.evio.0");
    gem_srs->SavePedestal("gem_ped.dat");
    gem_srs->SetPedestalMode(false);
    */

    for(auto &fec : gem_srs->GetFECList())
    {
        cout << "FEC " << fec->GetID() << ": " << endl;
        for(auto &apv : fec->GetAPVList())
        {
            cout << "     APV: " << apv->GetFECID() << ", " << apv->GetADCChannel() << endl;
        }
    }
// show the APVs and their strip numbers on planes
    // show the plane list
    auto det_list = gem_srs->GetDetectorList();

    for(auto &detector: det_list)
    {
        cout << "Detector: " << detector->GetName() << endl;
        for(auto &plane : detector->GetPlaneList())
        {
            cout << "    " << "Plane: " << plane->GetName() << endl;
            for(auto &apv : plane->GetAPVList())
            {
                cout << "    " << "    "
                     << "APV: " << apv->GetPlaneIndex()
                     << ", " << apv->GetAddress();

                int min = apv->GetPlaneStripNb(0);
                int max = apv->GetPlaneStripNb(0);
                for(size_t i = 1; i < apv->GetTimeSampleSize(); ++i)
                {
                    int strip = apv->GetPlaneStripNb(i);
                    if(min > strip) min = strip;
                    if(max < strip) max = strip;
                }

                cout << ", " << min
                     << " ~ " << max
                     << endl;
            }
        }
    }

    dst_parser->OpenInput("/work/hallb/prad/replay/prad_001287.dst");

    int count = 0;
    // uncomment next line, it will not update calibration factor from dst file

    while(dst_parser->Read() && count < 30000)
    {
        if(dst_parser->EventType() == PRad_DST_Event) {
            ++count;
            // you can push this event into data handler
            // handler->GetEventData().push_back(dst_parser->GetEvent()
            // or you can just do something with this event and discard it
            auto event = dst_parser->GetEvent();
            if(!event.is_physics_event())
                continue;

            gem_srs->Reconstruct(event);

            // detectors from GEM system
            for(auto &detector: gem_srs->GetDetectorList())
            {
                cout << "Detector: " << detector->GetName() << endl;
                // planes from a detector
                for(auto &plane : detector->GetPlaneList())
                {
                    cout << "    " << "Plane: " << plane->GetName() << endl;
                    // clusters from a plane
                    for(auto &cluster : plane->GetPlaneClusters())
                    {
                        cout << "    " << "    "
                             << "Cluster: "
                             << cluster.position << ", "
                             << cluster.peak_charge
                             << endl;
                        // hits from a cluster
                        for(auto &hit : cluster.hits)
                        {
                            cout << "    " << "    " << "     "
                                 << hit.strip << ", " << hit.charge << endl;
                        }
                    }
                }
            }
        }
    }

    dst_parser->CloseInput();

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << "Read " << handler->GetEventCount() << " events and "
         << handler->GetEPICSEventCount() << " EPICS events from file."
         << endl;
    cout << handler->GetBeamCharge() << endl;
    cout << handler->GetLiveTime() << endl;

    return 0;
}
