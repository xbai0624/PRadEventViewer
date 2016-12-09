//============================================================================//
// An example to test the performance of reconstruction methods               //
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

    dst_parser->OpenInput("prad_1310.dst");

    PRadBenchMark timer;

    int count = 0;
    while(dst_parser->Read())
    {
        if(dst_parser->EventType() == PRad_DST_Event) {

            auto event = dst_parser->GetEvent();
            if(!event.is_physics_event())
                continue;

            sys->Reconstruct(event);
            ++count;
        }
    }

    dst_parser->CloseInput();
    cout << "TIMER: Finished, read and reconstructed " << count << " events, "
         << "using method " << sys->GetClusterMethodName() << ", "
         << "took " << timer.GetElapsedTime() << " ms."
         << endl;

    return 0;
}
