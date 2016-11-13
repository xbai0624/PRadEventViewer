//============================================================================//
// An example showing how to use HyCAL clustering method to reconstruct data  //
//                                                                            //
// Chao Peng                                                                  //
// 11/12/2016                                                                 //
//============================================================================//

#include "PRadHyCalSystem.h"
#include "PRadBenchMark.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{
    PRadBenchMark timer;

    PRadHyCalSystem *sys = new PRadHyCalSystem("config/hycal.conf");
    //sys->ClearADCChannel();
    //sys->ClearTDCChannel();
    cout << sys->GetTDCList().size() << endl;
    cout << sys->GetADCList().size() << endl;
    for(auto adc : sys->GetADCList())
    {
        cout << "ADC: "
             << setw(6) << adc->GetName() << "  "
             << adc->GetAddress();
        if(adc->GetTDC())
            cout << setw(6) << adc->GetTDC()->GetName();
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
/*
    PRadHyCalDetector *hycal1 = sys->GetDetector();
    PRadHyCalDetector hycal = move(*hycal1);
    delete hycal1;
    for(auto module : hycal.GetModuleList())
    {
        cout << setw(4) << module->GetID() << ": " << *module << endl;
        if(module->GetChannel())
            cout << setw(6) << " " << module->GetChannel()->GetAddress() << endl;
    }
    cout << hycal.GetModuleList().size() << endl;
    //hycal.SortModuleList();
    //hycal.OutputModuleList(cout);
*/
    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;

    return 0;
}
