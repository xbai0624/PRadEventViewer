//============================================================================//
// An example showing how to get the cluster from HyCal and check how well it //
// could be described by cluster profile                                      //
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
void Evaluate(const string &file);
float EvalUniformity(const HyCalHit &hycal_hit, const vector<ModuleHit> &hits);

int main(int /*argc*/, char * /*argv*/ [])
{
    Evaluate("cosmic.dst");             // a cosimic run
    Evaluate("prad_1310.dst");          // a normal production run
    Evaluate("prad_1310_select.dst");   // selected events, (70%, 130%) beam energy
    return 0;
}

void Evaluate(const string &file)
{
    // remove affix
    string fname = file.substr(0, file.find_last_of("."));

    PRadDSTParser dst_parser;
    dst_parser.OpenInput(file);
    dst_parser.OpenOutput(fname + "_rej.dst");
    PRadDSTParser dst_parser2;
    dst_parser2.OpenOutput(fname + "_sav.dst");

    TFile f((fname + "_est.root").c_str(), "RECREATE");
    TH1F hist_cl("Likelihood_cl", "Estimator Cluster", 1000, 0., 50.);
    TH1F hist_ev("Likelihood_ev", "Estimator Event", 1000, 0., 50.);
    TH1F hist_uni("Uniformity", "Energy Fluctuation", 1000, 0., 3.);

    PRadHyCalSystem *sys = new PRadHyCalSystem("config/hycal.conf");
    PRadHyCalDetector *hycal = sys->GetDetector();
    PRadHyCalCluster *method = sys->GetClusterMethod();

    while(dst_parser.Read())
    {
        if(dst_parser.EventType() == PRad_DST_Event) {

            auto event = dst_parser.GetEvent();
            if(!event.is_physics_event())
                continue;

            sys->Reconstruct(event);

            auto &clusters = hycal->GetModuleClusters();
            float est = 0.;
            bool uniform = true;

            int count = 0;
            for(auto &cluster : clusters)
            {
                // it isn't a good cluster
                if(!method->CheckCluster(cluster))
                    continue;

                auto hit = method->Reconstruct(cluster);
                float cl_est = profile.EvalEstimator(hit, cluster);
                hist_cl.Fill(cl_est);
                est += cl_est;
                ++count;

                // found a not uniform cluster
                float uni = EvalUniformity(hit, cluster.hits);
                hist_uni.Fill(uni);
                if(uni > 0.8)
                    uniform = false;
            }

            est /= count;
            hist_ev.Fill(est);

            // too uniform, reject
            if(uniform)
                dst_parser.WriteEvent(event);
            else
                dst_parser2.WriteEvent(event);
        }
    }

    dst_parser.CloseInput();
    dst_parser.CloseOutput();
    dst_parser2.CloseOutput();
    hist_cl.Write();
    hist_ev.Write();
    hist_uni.Write();
    f.Close();
}

float EvalUniformity(const HyCalHit &hycal_hit, const vector<ModuleHit> &hits)
{
    float est = 0.;
    float avg_energy = hycal_hit.E/hits.size();
    for(auto hit : hits)
    {
        float diff = fabs(hit.energy - avg_energy);
        // log likelyhood for double exponential distribution
        est += fabs(diff)/avg_energy;
    }

    return est/hits.size();
}
