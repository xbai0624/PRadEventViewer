//============================================================================//
// An example showing how to get beam charge information from one run         //
//                                                                            //
// Chao Peng                                                                  //
// 11/12/2016                                                                 //
//============================================================================//

#include "PRadDSTParser.h"
#include "PRadEventFilter.h"
#include "PRadInfoCenter.h"
#include "ConfigParser.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#define PROGRESS_COUNT 10000

using namespace std;

void ChargeCount(const string &file, ofstream &out);

// expecting two input string, event file path and bad events list path
int main(int argc, char *argv[])
{
    if(argc < 2)
        return 0;

    ofstream output("beam_charge.dat", ios::app);
    for(int i = 1; i < argc; ++i)
    {
        string file = argv[i];
        ChargeCount(file, output);
    }
    output.close();
}

void ChargeCount(const string &file, ofstream &out)
{
    cout << "Beam Charge Counting for " << "\"" << file << "\"."
         << endl;

    PRadInfoCenter::Instance().Reset();
    PRadInfoCenter::SetRunNumber(file);

    PRadDSTParser dst_parser;
    dst_parser.OpenInput(file);

    while(dst_parser.Read())
    {
        if(dst_parser.EventType() == PRad_DST_Event) {
            auto event = dst_parser.GetEvent();
            PRadInfoCenter::Instance().UpdateInfo(event);
        }
    }
    dst_parser.CloseInput();

    cout << "TIMER: Finished beam charge counting,"
         << " total beam charge: " << PRadInfoCenter::GetBeamCharge() << " nC"
         << " average live time: " << PRadInfoCenter::GetLiveTime()*100. << "%"
         << " accepted beam charge: " << PRadInfoCenter::GetLiveBeamCharge() << " nC "
         << endl;

    out << setw(6) << PRadInfoCenter::GetRunNumber()
        << setw(10) << PRadInfoCenter::GetBeamCharge()
        << setw(10) << PRadInfoCenter::GetLiveTime()
        << setw(10) << PRadInfoCenter::GetLiveBeamCharge()
        << endl;
}
