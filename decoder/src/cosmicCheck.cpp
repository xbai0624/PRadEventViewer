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
#include "TFile.h"
#include "TH1.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace std;

// reference to a signleton instance
const PRadClusterProfile &profile = PRadClusterProfile::Instance();

// declaration of functions
float EvalEstimator(const HyCalHit &hit, const std::vector<ModuleHit> &hits);


int main(int /*argc*/, char * /*argv*/ [])
{
    PRadHyCalSystem *sys = new PRadHyCalSystem("config/hycal.conf");
    PRadDSTParser *dst_parser = new PRadDSTParser();

    dst_parser->OpenInput("cosmic.dst");
    dst_parser->OpenOutput("rejection.dst");
    PRadHyCalDetector *hycal = sys->GetDetector();
    PRadHyCalCluster *method = sys->GetClusterMethod();

    TFile *f = new TFile("cosmic.root", "RECREATE");
    TH1F *hist = new TH1F("Likelihood", "Estimator", 1000, 0., 20.);

    PRadBenchMark timer;

    while(dst_parser->Read())
    {
        if(dst_parser->EventType() == PRad_DST_Event) {

            auto event = dst_parser->GetEvent();
            if(!event.is_physics_event())
                continue;

            sys->Reconstruct(event);

            auto &clusters = hycal->GetModuleClusters();

            for(auto &cluster : clusters)
            {
                // it isn't a good cluster
                if(!method->CheckCluster(cluster))
                    continue;

                auto hit = method->Reconstruct(cluster);
                float est = EvalEstimator(hit, cluster.hits);
                hist->Fill(est);
                if(est > 10)
                    dst_parser->WriteEvent(event);
            }
        }
    }

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    hist->Write();
    f->Close();

    return 0;
}

float EvalEstimator(const HyCalHit &hycal_hit, const std::vector<ModuleHit> &hits)
{
    float est = 0.;
    for(auto hit : hits)
    {
        int dx = abs((hit.geo.x - hycal_hit.x)/hit.geo.size_x * 100.);
        int dy = abs((hit.geo.y - hycal_hit.y)/hit.geo.size_y * 100.);
        float frac = profile.GetFraction(hit.geo.type, dx, dy);
        float err = profile.GetError(hit.geo.type, dx, dy);

        float diff = fabs(hit.energy - hycal_hit.E*frac);
        float chi = sqrt(0.816*hit.energy + 2.6*sqrt(hycal_hit.E/1000.)*err);

        // log likelyhood for double exponential distribution
        est += diff/chi;
    }

    return est/hits.size();
}
