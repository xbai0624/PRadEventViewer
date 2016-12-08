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
    PRadHyCalSystem *sys = new PRadHyCalSystem("config/hycal.conf");
    PRadDSTParser *dst_parser = new PRadDSTParser();

    // show adc and tdc channels
    /*
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
    // show the modules
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
    */

    // test reconstruction performance
//    dst_parser->OpenInput("/work/hallb/prad/replay/prad_001288.dst");
    dst_parser->OpenInput("prad_1310.dst");
    dst_parser->OpenOutput("prad_1310_leak.dst");
    PRadBenchMark timer;

    while(dst_parser->Read())
    {
        if(dst_parser->EventType() == PRad_DST_Event) {

            auto event = dst_parser->GetEvent();
            if(!event.is_physics_event())
                continue;

            sys->Reconstruct(event);
            bool save = false;
            for(auto hit : sys->GetDetector()->GetHits())
            {
                if(TEST_BIT(hit.flag, kDeadModule)) {
                    save = true;
                    break;
                }
            }

            if(save)
                dst_parser->WriteEvent(event);
        }
    }

    dst_parser->CloseOutput();
    dst_parser->CloseInput();
    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;

    return 0;
}
