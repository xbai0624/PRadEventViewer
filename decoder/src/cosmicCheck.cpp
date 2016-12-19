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

#define MOST_UNIF_EST 0.8
#define MOST_PROF_EST 9.0
#define PROGRESS_COUNT 10000

// reference to a signleton instance
const PRadClusterProfile &profile = PRadClusterProfile::Instance();

// declaration of functions
void Evaluate(const string &file);
float EvalUniformity(const HyCalHit &hycal_hit, const vector<ModuleHit> &hits);
ostream &operator <<(ostream &os, const PRadBenchMark &timer);

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        cout << "usage: cosmicCheck <file1> <file2> ..." << endl;
        return 0;
    }

    for(int i = 1; i < argc; ++i)
    {
        string file = argv[i];
        cout << "Analyzing File " << file << "..." << endl;
        Evaluate(file);          // a normal production run
    }

    return 0;
}

void Evaluate(const string &file)
{
    // remove directory and affix
    const string &fname = ConfigParser::decompose_path(file).name;

    PRadDSTParser dst_parser;
    dst_parser.OpenInput(file);
    dst_parser.OpenOutput(fname + "_sav.dst");
    PRadDSTParser dst_parser2;
    dst_parser2.OpenOutput(fname + "_rej.dst");

    TFile f((fname + "_prof.root").c_str(), "RECREATE");
    TH1F hist_cl("Likelihood_cl", "Estimator Cluster", 1000, 0., 50.);
    TH1F hist_ev("Likelihood_ev", "Estimator Event", 1000, 0., 50.);
    TH1F hist_uni("Uniformity", "Energy Uniformity", 1000, 0., 5.);

    PRadHyCalSystem *sys = new PRadHyCalSystem("config/hycal.conf");
    PRadHyCalDetector *hycal = sys->GetDetector();
    PRadHyCalCluster *method = sys->GetClusterMethod();

    int count = 0;
    PRadBenchMark timer;
    while(dst_parser.Read())
    {
        if(dst_parser.EventType() == PRadDSTParser::Type::event) {

            count++;
            if(count%PROGRESS_COUNT == 0) {
                cout <<"----------event " << count
                     << "-------[ " << timer << " ]------"
                     << "\r" << flush;
            }

            auto event = dst_parser.GetEvent();

            // save sync event no matter what it is
            if(event.is_sync_event())
            {
                dst_parser.WriteEvent(event);
                continue;
            }

            // discard non-physics events
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
                if(uni > MOST_UNIF_EST)
                    uniform = false;
            }

            est /= count;
            hist_ev.Fill(est);

            // too uniform or too bad profile, reject
            if(uniform || est > MOST_PROF_EST)
                dst_parser2.WriteEvent(event);
            else
                dst_parser.WriteEvent(event);

        } else if (dst_parser.EventType() == PRadDSTParser::Type::epics) {
            dst_parser.WriteEPICS();
        }
    }

    cout <<"----------event " << count
         << "-------[ " << timer << " ]------"
         << endl;

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

ostream &operator <<(ostream &os, const PRadBenchMark &timer)
{
    int t_sec = timer.GetElapsedTime()/1000;
    int hour = t_sec/3600;
    int min = (t_sec%3600)/60;
    int sec = (t_sec%3600)%60;

    os << hour << " hr "
       << min << " min "
       << sec << " sec";

    return os;
}
