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
#include "TFile.h"
#include "TH1.h"

using namespace std;

struct DataPoint
{
    float x;
    float y;
    float E;

    DataPoint() {};
    DataPoint(float xi, float yi, float Ei) : x(xi), y(yi), E(Ei) {};
};

typedef pair<DataPoint, DataPoint> MollerEvent;
typedef vector<MollerEvent> MollerData;

void AnalyzeMollerCenter(const string &path);
void FillMollerEvents(const string &dst_path, MollerData &data);
DataPoint GetIntersection(const MollerEvent &m1, const MollerEvent &m2);

int main(int argc, char *argv[])
{
    if(argc < 2)
        return 0;

    for(int i = 1; i < argc; ++i)
    {
        string file = argv[i];
        AnalyzeMollerCenter(file);
    }

    return 0;
}

void AnalyzeMollerCenter(const string &path)
{
    MollerData mollers;
    FillMollerEvents(path, mollers);

    auto dir_path = ConfigParser::decompose_path(path);

    TFile f((dir_path.second + "_mc.root").c_str(), "RECREATE");
    TH1F x_hist("x", "Center_X", 1000, -100., 100.);
    TH1F y_hist("y", "Center_Y", 1000, -100., 100.);

    auto it = mollers.begin();
    auto it_next = it + 1;

    for(; it != mollers.end() && it_next != mollers.end(); it +=2, it_next +=2)
    {
        auto inter = GetIntersection(*it, *it_next);
        x_hist.Fill(inter.x);
        y_hist.Fill(inter.y);
    }

    x_hist.Write();
    y_hist.Write();
    f.Close();
}

void FillMollerEvents(const string &dst_path, MollerData &data)
{
    PRadEPICSystem *epics = new PRadEPICSystem("config/epics_channels.conf");
    PRadHyCalSystem *hycal = new PRadHyCalSystem("config/hycal.conf");
    PRadGEMSystem *gem = new PRadGEMSystem("config/gem.conf");

    PRadDSTParser *dst_parser = new PRadDSTParser();
    dst_parser->SetMode(No_Run_Info_Update);
    // coordinate system and detector match system
    PRadCoordSystem *coord_sys = new PRadCoordSystem("database/coordinates.dat");
    PRadDetMatch *det_match = new PRadDetMatch("config/det_match.conf");

    PRadBenchMark timer;

    // here shows an example how to read DST file while not saving all the events
    // in memory
    dst_parser->OpenInput(dst_path);
    // determine run number from file name
    PRadInfoCenter::SetRunNumber(dst_path);
    // choose correct coordinates offset
    coord_sys->ChooseCoord(PRadInfoCenter::GetRunNumber());

    PRadHyCalDetector *hycal_det = hycal->GetDetector();
    PRadGEMDetector *gem_det1 = gem->GetDetector("PRadGEM1");
    PRadGEMDetector *gem_det2 = gem->GetDetector("PRadGEM2");

    while(dst_parser->Read())
    {
        if(dst_parser->EventType() == PRad_DST_Event) {
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
            coord_sys->Transform(hycal_det->GetDetID(), hycal_hit.begin(), hycal_hit.end());
            coord_sys->Transform(gem_det1->GetDetID(), gem1_hit.begin(), gem1_hit.end());
            coord_sys->Transform(gem_det2->GetDetID(), gem2_hit.begin(), gem2_hit.end());
            // or arrays
            coord_sys->Projection(hycal_hit.begin(), hycal_hit.end());
            coord_sys->Projection(gem1_hit.begin(), gem1_hit.end());
            coord_sys->Projection(gem2_hit.begin(), gem2_hit.end());

            // hits matching, return matched index
            auto matched = det_match->Match(hycal_hit, gem1_hit, gem2_hit);

            // we need clean double arm Moller
            if(matched.size() != 2)
                continue;

            const HyCalHit &h1 = hycal_hit.at(matched.at(0).hycal);
            const GEMHit &g1 = (matched.at(0).gem1 < 0)?
                               gem2_hit.at(matched.at(0).gem2) :
                               gem1_hit.at(matched.at(0).gem1);

            const HyCalHit &h2 = hycal_hit.at(matched.at(1).hycal);
            const GEMHit &g2 = (matched.at(1).gem1 < 0)?
                               gem2_hit.at(matched.at(1).gem2) :
                               gem1_hit.at(matched.at(1).gem1);

            // two moller points
            DataPoint moller1(g1.x, g1.y, h1.E);
            DataPoint moller2(g2.x, g2.y, h2.E);

            bool good_moller = true;
            float beam_energy = 1097;
            // check if energy is good
            if(fabs(h1.E + h2.E - beam_energy) >= 0.2*beam_energy)
                good_moller = false;

            if(good_moller)
                data.push_back(make_pair(moller1, moller2));

        } else if(dst_parser->EventType() == PRad_DST_Epics) {
            // save epics into handler, otherwise get epicsvalue won't work
            epics->AddEvent(dst_parser->GetEPICSEvent());
        }
    }

    dst_parser->CloseInput();
}

DataPoint GetIntersection(const MollerEvent &m1, const MollerEvent &m2)
{
    DataPoint inter;

    // get line slope and intercept for m1
    float k1 = (m1.first.y - m1.second.y)/(m1.first.x - m1.second.x);
    float b1 = m1.first.y - k1*m1.first.x;

    // get line slop and intercept for m2
    float k2 = (m2.first.y - m2.second.y)/(m2.first.x - m2.second.x);
    float b2 = m2.first.y - k2*m2.first.x;

    inter.x = (b1 - b2)/(k2 - k1);
    inter.y = k1*inter.x + b1;

    return inter;
}
