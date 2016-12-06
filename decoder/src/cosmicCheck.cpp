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
void Evaluate(const string &file, PRadHyCalSystem *sys);
float EvalEstimator(const HyCalHit &hit, const vector<ModuleHit> &hits);
float EvalUniformity(const HyCalHit &hycal_hit, const vector<ModuleHit> &hits);

int main(int /*argc*/, char * /*argv*/ [])
{
    PRadHyCalSystem *sys = new PRadHyCalSystem("config/hycal.conf");
    Evaluate("cosmic.dst", sys);            // a cosimic run
    Evaluate("prad_1310.dst", sys);         // a normal production run
    Evaluate("prad_1310_select.dst", sys);  // selected events, (70%, 130%) beam energy
    return 0;
}

void Evaluate(const string &file, PRadHyCalSystem *sys)
{
    // remove affix
    string fname = file.substr(0, file.find_last_of("."));

    PRadDSTParser dst_parser;
    dst_parser.OpenInput(file);
    dst_parser.OpenOutput(fname + "_rej.dst");

    TFile f((fname + "_est.root").c_str(), "RECREATE");
    TH1F hist("Likelihood", "Estimator", 1000, 0., 50.);
    TH1F hist_uni("Uniformity", "Energy Fluctuation", 1000, 0., 3.);

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

            for(auto &cluster : clusters)
            {
                // it isn't a good cluster
                if(!method->CheckCluster(cluster))
                    continue;

                auto hit = method->Reconstruct(cluster);
                est += EvalEstimator(hit, cluster.hits);

                // found a not uniform cluster
                float uni = EvalUniformity(hit, cluster.hits);
                hist_uni.Fill(uni);
                if(uni > 1.)
                    uniform = false;
            }

            est /= clusters.size();
            hist.Fill(est);

            // too uniform, reject
            if(uniform)
                dst_parser.WriteEvent(event);
        }
    }

    dst_parser.CloseInput();
    dst_parser.CloseOutput();

    hist.Write();
    hist_uni.Write();
    f.Close();
}

inline float quad_sum(const float &x, const float &y)
{
    return x*x + y*y;
}

float EvalEstimator(const HyCalHit &hycal_hit, const vector<ModuleHit> &hits)
{
    float est = 0.;
    for(auto hit : hits)
    {
        const auto &prof = profile.GetProfile(hycal_hit.x, hycal_hit.y, hit.geo.x, hit.geo.y);

        float diff = fabs(hit.energy - hycal_hit.E*prof.frac);

        float res = 0.026;  // 2.6% for PbWO4
        if(TEST_BIT(hycal_hit.flag, kPbGlass))
            res = 0.065;    // 6.5% for PbGlass
        if(TEST_BIT(hycal_hit.flag, kTransition))
            res = 0.050;    // 5.0% for transition

        // get resolution
        res /= sqrt(hycal_hit.E/1000.);

        // energy resolution part and profile error part
//        float sigma2 = quad_sum(res*hit.energy, prof.frac*hycal_hit.E*prof.err);
        float sigma2 = 0.816*hit.energy + res*hycal_hit.E*prof.err;

        // log likelyhood for double exponential distribution
        est += diff/sqrt(sigma2);
    }

    return est/hits.size();
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
