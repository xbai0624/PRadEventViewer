//============================================================================//
// An example showing how to use HyCAL clustering method to reconstruct data  //
//                                                                            //
// Chao Peng                                                                  //
// 11/12/2016                                                                 //
//============================================================================//

#include "PRadHyCalSystem.h"
#include "PRadBenchMark.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{
    PRadBenchMark timer;

    PRadHyCalSystem *sys = new PRadHyCalSystem("hycal.conf");

    PRadHyCalDetector *hycal1 = sys->GetDetector();
    PRadHyCalDetector hycal = move(*hycal1);
    delete hycal1;
    for(auto module : hycal.GetModuleList())
    {
        cout << module->GetID() << ": " << *module << endl;
    }
    cout << hycal.GetModuleList().size() << endl;
    //hycal.SortModuleList();
    //hycal.OutputModuleList(cout);

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;

    return 0;
}
