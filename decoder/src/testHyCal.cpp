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
    PRadHyCalSystem sys2 = move(*sys);
    delete sys;
    sys = &sys2;
    //sys->ClearADCChannel();
    //sys->ClearTDCChannel();
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
    //hycal.SortModuleList();
    //hycal.OutputModuleList(cout);
    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;

    return 0;
}
