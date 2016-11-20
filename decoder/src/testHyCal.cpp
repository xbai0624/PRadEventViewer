//============================================================================//
// An example showing how to use HyCAL clustering method to reconstruct data  //
//                                                                            //
// Chao Peng                                                                  //
// 11/12/2016                                                                 //
//============================================================================//

#include "PRadHyCalSystem.h"
#include "PRadDataHandler.h"
#include "PRadDSTParser.h"
#include "PRadBenchMark.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{
    PRadBenchMark timer;


    PRadDataHandler *handler = new PRadDataHandler();
    PRadHyCalSystem *sys = new PRadHyCalSystem("config/hycal.conf");
    handler->SetHyCalSystem(sys);
    PRadDSTParser *dst_parser = new PRadDSTParser(handler);

    //sys->ClearADCChannel();
    //sys->ClearTDCChannel();
    /*
    cout << sys->GetTDCList().size() << endl;
    cout << sys->GetADCList().size() << endl;
    for(auto adc : sys->GetADCList())
    {
        cout << "ADC: "
             << *adc;
        if(adc->GetTDC())
            cout << *adc->GetTDC();
        cout << endl;
    }
    for(auto tdc : sys->GetTDCList())
    {
        cout << "TDC: "
             << setw(6) << tdc->GetName() << "  "
             << tdc->GetAddress()
             << endl;
        for(auto adc : tdc->GetChannelList())
        {
            cout << setw(6) << adc->GetName() << ",";
        }
        cout << endl;
    }
    */
/*
    PRadHyCalDetector *hycal = sys->GetDetector();
    for(auto module : hycal->GetModuleList())
    {
        cout << setw(4) << module->GetID() << ": " << *module;
        cout << setw(12) << module->GetNonLinearConst();
        if(module->GetChannel())
            cout << module->GetChannel()->GetAddress();
        cout << endl;
    }
    cout << hycal->GetModuleList().size() << endl;
*/
    /*
    hycal->SortModuleList();
    ofstream output("hycal_module.txt");
    hycal->OutputModuleList(output);
    */
    dst_parser->OpenInput("/work/hallb/prad/replay/prad_001288.dst");
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

            sys->Reconstruct(event);

            for(auto cluster : sys->GetDetector()->GetCluster())
            {
                cout << cluster.E << ", " << cluster.x << ", " << cluster.y << endl;
            }
        }
    }

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;

    return 0;
}
