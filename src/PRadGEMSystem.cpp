//============================================================================//
// GEM System class                                                           //
// It has both the physical and the DAQ structure of GEM detectors in PRad    //
// Physical structure: 2 detectors, each has 2 planes (X, Y)                  //
// DAQ structure: several FECs, each has several APVs                         //
// APVs and detector planes are connected                                     //
//                                                                            //
// Structure of the DAQ system is based on the code from Kondo Gnanvo         //
// and Xinzhan Bai                                                            //
//                                                                            //
// Chao Peng                                                                  //
// 06/17/2016                                                                 //
//============================================================================//

#include "PRadGEMSystem.h"
#include "ConfigParser.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "TFile.h"
#include "TH1.h"

using namespace std;



//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//

// constructor
PRadGEMSystem::PRadGEMSystem(const string &config_file, int daq_cap, int det_cap)
: gem_recon(new PRadGEMCluster()), PedestalMode(false)
{
    daq_slots.resize(daq_cap, nullptr);
    det_slots.resize(det_cap, nullptr);

    if(!config_file.empty())
        LoadConfiguration(config_file);
}

// the copy and move constructor will not only copy all the members that managed
// by GEM System, but also build the connections between them
// copy/move constructor
PRadGEMSystem::PRadGEMSystem(const PRadGEMSystem &that)
{
    gem_recon = new PRadGEMCluster(*that.gem_recon);
    //TODO
}

PRadGEMSystem::PRadGEMSystem(PRadGEMSystem &&that)
{
    //TODO
}

// desctructor
PRadGEMSystem::~PRadGEMSystem()
{
    Clear();
    delete gem_recon;
}

PRadGEMSystem &PRadGEMSystem::operator =(const PRadGEMSystem &rhs)
{
    //TODO
    return *this;
}

PRadGEMSystem &PRadGEMSystem::operator =(PRadGEMSystem &&rhs)
{
    //TODO
    return *this;
}



//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

void PRadGEMSystem::Clear()
{
    for(auto &fec : daq_slots)
    {
        // this prevent fec from calling RemoveFEC upon destruction
        if(fec != nullptr)
            fec->UnsetSystem(true);
        delete fec, fec = nullptr;
    }

    fec_list.clear();

    for(auto &det : det_slots)
    {
        if(det != nullptr)
            det->UnsetSystem(true);
        delete det, det = nullptr;
    }

    det_list.clear();
    det_name_map.clear();
}

void PRadGEMSystem::LoadConfiguration(const string &path) throw(PRadException)
{
    // release memory before load new configuration
    Clear();

    ConfigParser c_parser;
    c_parser.SetSplitters(",");

    if(!c_parser.OpenFile(path)) {
        throw PRadException("GEM System", "cannot open configuration file " + path);
    }

    // we accept 4 types of elements
    // detector, plane, fec, apv
    vector<string> types = {"DET", "PLN", "FEC", "APV"};
    // this vector is to store all the following arguments
    vector<queue<ConfigValue>> args[types.size()];

    // read all the elements in
    while(c_parser.ParseLine())
    {
        string key = c_parser.TakeFirst();
        size_t i = 0;
        for(; i < types.size(); ++i)
        {
            if(ConfigParser::strcmp_case_insensitive(key, types.at(i))) {
                args[i].push_back(c_parser.TakeAll());
                break;
            }
        }

        if(i >= types.size()) { // did not find any type
            cout << "PRad GEM System Warning: Undefined element type " << key
                 << " in configuration file "
                 << "\"" << path << "\""
                 << endl;
        }
    }

    // order is very important,
    // since planes will be added to detectors
    // and apv will be added to fecs and connected to planes
    // add detectors and planes
    for(auto &det : args[0])
        buildDetector(det);
    for(auto &pln : args[1])
        buildPlane(pln);
    // add fecs and apvs
    for(auto &fec : args[2])
        buildFEC(fec);
    for(auto &apv : args[3])
        buildAPV(apv);
}

void PRadGEMSystem::LoadClusterConfiguration(const string &path)
{
    gem_recon->Configure(path);
}

void PRadGEMSystem::LoadPedestal(const string &path) throw(PRadException)
{
    ConfigParser c_parser;
    c_parser.SetSplitters(",: \t");

    if(!c_parser.OpenFile(path)) {
        throw PRadException("GEM System", "cannot open pedestal data file " + path);
    }

    PRadGEMAPV *apv = nullptr;

    while(c_parser.ParseLine())
    {
        ConfigValue first = c_parser.TakeFirst();

        if(first == "APV") { // a new APV

            int fec, adc;
            c_parser >> fec >> adc;
            apv = GetAPV(fec, adc);

        } else { // different adc channel in this APV

            float offset, noise;
            c_parser >> offset >> noise;

            if(apv)
                apv->UpdatePedestal(PRadGEMAPV::Pedestal(offset, noise), first.Int());

        }
    }
}

// return true if the detector is successfully registered
bool PRadGEMSystem::Register(PRadGEMDetector *det)
{
    if(det == nullptr)
        return false;

    if((size_t)det->GetDetID() >= det_slots.size())
    {
        cerr << "GEM System Error: Failed to add detector "
             << det->GetName()
             << " (" << det->GetDetID() << ")"
             << ", exceeds the maximum capacity."
             << endl;
        return false;
    }

    if(det_slots.at(det->GetDetID()) != nullptr)
    {
        cerr << "GEM System Error: Failed to add detector "
             << det->GetName()
             << " (" << det->GetDetID() << ")"
             << ", the same detector already exists in the system."
             << endl;
        return false;
    }

    det->SetSystem(this);
    det_slots[det->GetDetID()] = det;
    return true;
}

// return true if the fec is successfully registered
bool PRadGEMSystem::Register(PRadGEMFEC *fec)
{
    if(fec == nullptr)
        return false;

    if((size_t)fec->GetID() >= daq_slots.size())
    {
        cerr << "GEM System Error: Failed to add FEC id " << fec->GetID()
             << ", it exceeds current DAQ capacity " << daq_slots.size()
             << "."
             << endl;
        return false;
    }

    if(daq_slots.at(fec->GetID()) != nullptr)
    {
        cerr << "GEM System Error: Failed to add FEC id " << fec->GetID()
             << ", FEC with the same id already exists in the system."
             << endl;
        return false;
    }

    fec->SetSystem(this);
    daq_slots[fec->GetID()] = fec;
    return true;
}

void PRadGEMSystem::RemoveDetector(int det_id)
{
    if((size_t)det_id < det_slots.size())
        det_slots[det_id] = nullptr;

    // rebuild maps
    RebuildDetectorMap();
}

void PRadGEMSystem::RemoveFEC(int fec_id)
{
    if((size_t)fec_id < daq_slots.size())
        daq_slots[fec_id] = nullptr;

    // rebuild maps
    RebuildDAQMap();
}

void PRadGEMSystem::RebuildDetectorMap()
{
    det_list.clear();
    det_name_map.clear();

    for(auto &det : det_slots)
    {
        if(det == nullptr)
            continue;

        det_list.push_back(det);
        det_name_map[det->GetName()] = det;
    }
}

void PRadGEMSystem::RebuildDAQMap()
{
    fec_list.clear();

    for(auto &fec : daq_slots)
    {
        if(fec == nullptr)
            continue;

        fec_list.push_back(fec);
    }
}

PRadGEMDetector *PRadGEMSystem::GetDetector(const int &det_id)
const
{
    if((unsigned int) det_id >= det_slots.size()) {
        return nullptr;
    }
    return det_slots[det_id];
}

PRadGEMDetector *PRadGEMSystem::GetDetector(const string &name)
const
{
    auto it = det_name_map.find(name);
    if(it == det_name_map.end()) {
        cerr << "GEM System: Cannot find detector " << name << endl;
        return nullptr;
    }
    return it->second;
}

PRadGEMFEC *PRadGEMSystem::GetFEC(const int &id)
const
{
    if((size_t)id < daq_slots.size()) {
        return daq_slots[id];
    }

    return nullptr;
}

PRadGEMAPV *PRadGEMSystem::GetAPV(const int &fec_id, const int &apv_id)
const
{
    if (((size_t) fec_id < daq_slots.size()) &&
        (daq_slots.at(fec_id) != nullptr)) {

        return daq_slots.at(fec_id)->GetAPV(apv_id);
    }

    return nullptr;
}

PRadGEMAPV *PRadGEMSystem::GetAPV(const GEMChannelAddress &addr)
const
{
    return GetAPV(addr.fec_id, addr.adc_ch);
}

void PRadGEMSystem::FillRawData(GEMRawData &raw, vector<GEM_Data> &container, const bool &fill_hist)
{
    PRadGEMAPV *apv = GetAPV(raw.addr);

    if(apv != nullptr) {

        apv->FillRawData(raw.buf, raw.size);

        if(fill_hist) {
            if(PedestalMode)
                apv->FillPedHist();
        } else {
            apv->ZeroSuppression();
#ifdef MULTI_THREAD
            locker.lock();
#endif
            apv->CollectZeroSupHits(container);
#ifdef MULTI_THREAD
            locker.unlock();
#endif
        }
    }
}

void PRadGEMSystem::ClearAPVData()
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::ClearData);
    }
}

void PRadGEMSystem::FillZeroSupData(vector<GEMZeroSupData> &data_pack,
                                    vector<GEM_Data> &container)
{
    // clear all the APVs' hits
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::ResetHitPos);
    }

    // fill in the online zero-suppressed data
    for(auto &data : data_pack)
        FillZeroSupData(data);

    // collect these zero-suppressed hits
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::CollectZeroSupHits, container);
    }
}

void PRadGEMSystem::FillZeroSupData(GEMZeroSupData &data)
{
    PRadGEMAPV *apv = GetAPV(data.addr);

    if(apv != nullptr) {
        apv->FillZeroSupData(data.channel, data.time_sample, data.adc_value);
    }
}

void PRadGEMSystem::ChooseEvent(const EventData &data)
{
    // clear all the APVs' hits
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::ClearData);
    }

    for(auto &hit : data.gem_data)
    {
        auto apv = GetAPV(hit.addr.fec, hit.addr.adc);
        if(apv)
            apv->FillZeroSupData(hit.addr.strip, hit.values);
    }
}

void PRadGEMSystem::Reconstruct(const EventData &data)
{
    // only reconstruct physics event
    if(!data.is_physics_event())
        return;

    // Plane based reconstruction, collect hits first
    // There exists two way to collect hits to plane.
    // This is the first way, collect hits from event data:
    // 1. Get hits data
    // 2. Find corresponding APV and its connected plane
    // 3. Transform APV's channel to plane strip
    // 4. Add plane hits (plane strip, charges from time samples)
    // The other way is
    // Decode the raw data, do zero suppression on all APVs
    // or use ChooseEvent() to redistribute the hits to APVs
    // so APV got the hits in its storage
    // use plane->CollectAPVHits() to ccollect these hits

    // clear hits for all detector planes
    for(auto &det : det_list)
    {
        det->ClearHits();
    }

    // add the hits from event data
    for(auto &hit : data.gem_data)
    {
        auto apv = GetAPV(hit.addr.fec, hit.addr.adc);
        if(apv == nullptr)
            continue;

        auto plane = apv->GetPlane();
        if(plane == nullptr) {
            cout << "GEM System Warning: APV " << apv->GetAddress()
                 << " is not connected to any detector plane."
                 << endl;
            continue;
        }

        plane->AddPlaneHit(apv->GetPlaneStripNb(hit.addr.strip), hit.values);
    }

    for(auto &det : det_list)
    {
        det->ReconstructHits(gem_recon);
    }
}

void PRadGEMSystem::FitPedestal()
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::FitPedestal);
    }
}

void PRadGEMSystem::SavePedestal(const string &name)
const
{
    ofstream in_file(name);

    if(!in_file.is_open()) {
        cerr << "GEM System: Failed to save pedestal, file "
             << name << " cannot be opened."
             << endl;
    }

    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::PrintOutPedestal, in_file);
    }
}

void PRadGEMSystem::SetPedestalMode(const bool &m)
{
    PedestalMode = m;

    for(auto &fec : fec_list)
    {
        if(m)
            fec->APVControl(&PRadGEMAPV::CreatePedHist);
        else
            fec->APVControl(&PRadGEMAPV::ReleasePedHist);
    }
}

vector<GEM_Data> PRadGEMSystem::GetZeroSupData()
const
{
    vector<GEM_Data> gem_data;

    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::CollectZeroSupHits, gem_data);
    }

    return gem_data;
}

void PRadGEMSystem::SetUnivCommonModeThresLevel(const float &thres)
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::SetCommonModeThresLevel, thres);
    }
}

void PRadGEMSystem::SetUnivZeroSupThresLevel(const float &thres)
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::SetZeroSupThresLevel, thres);
    }
}

void PRadGEMSystem::SetUnivTimeSample(const size_t &ts)
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::SetTimeSample, ts);
    }
}

void PRadGEMSystem::SaveHistograms(const string &path)
const
{
    TFile *f = new TFile(path.c_str(), "recreate");

    for(auto fec : fec_list)
    {
        string fec_name = "FEC " + to_string(fec->GetID());
        TDirectory *cdtof = f->mkdir(fec_name.c_str());
        cdtof->cd();

        for(auto apv : fec->GetAPVList())
        {
            string adc_name = "ADC " + to_string(apv->GetADCChannel());
            TDirectory *cur_dir = cdtof->mkdir(adc_name.c_str());
            cur_dir->cd();

            for(auto hist : apv->GetHistList())
                hist->Write();
        }
    }

    f->Close();
    delete f;
}

vector<PRadGEMAPV *> PRadGEMSystem::GetAPVList()
const
{
    vector<PRadGEMAPV *> list;
    for(auto &fec : fec_list)
    {
        vector<PRadGEMAPV *> sub_list = fec->GetAPVList();
        list.insert(list.end(), sub_list.begin(), sub_list.end());
    }

    return list;
}

//============================================================================//
// Private Member Functions                                                   //
//============================================================================//

bool PRadGEMSystem::checkArgs(const string &type, size_t size, size_t expect)
{
    if(size != expect) {
        cout << "PRad GEM System Warning: Fail to load " << type
             << ", expected " << expect << " arguments, "
             << "received " << size << " arguments."
             << endl;
        return false;
    }

    return true;
}

// defined operator for easier arguments reading
template<typename T>
queue<ConfigValue> &operator >>(queue<ConfigValue> &lhs, T &t)
{
    t = lhs.front().Convert<T>();
    lhs.pop();
    return lhs;
}

// build the detectors and planes according to the arguments
void PRadGEMSystem::buildDetector(queue<ConfigValue> &det_args)
{
    if(!checkArgs("DET", det_args.size(), 3))
        return;

    string readout, type, name;
    det_args >> readout >> type >> name;
    PRadGEMDetector *new_det = new PRadGEMDetector(readout, type, name);

    if(!Register(new_det)) {
        delete new_det;
        return;
    }

    // successfully registered, build maps
    det_list.push_back(new_det);
    det_name_map[name] = new_det;
}

void PRadGEMSystem::buildPlane(queue<ConfigValue> &pln_args)
{
    if(!checkArgs("PLN", pln_args.size(), 6))
        return;

    string det_name, plane_name;
    double size;
    int type, connector, orient, direct;

    pln_args >> det_name >> plane_name >> size >> connector >> orient >> direct;
    type = PRadGEMPlane::GetPlaneTypeID(plane_name.c_str());

    if(type < 0) // did not find proper type
        return;

    PRadGEMDetector *det = GetDetector(det_name);
    if(det == nullptr) {// did not find detector
        cout << "PRad GEM System Warning: Failed to add plane " << plane_name
             << " to detector " << det_name
             << ", please check if the detector exists in GEM configuration file."
             << endl;
        return;
    }

    PRadGEMPlane *new_plane = new PRadGEMPlane(plane_name, type, size, connector, orient, direct);

    // failed to add plane
    if(!det->AddPlane(new_plane)) {
        delete new_plane;
        return;
    }
}

void PRadGEMSystem::buildFEC(queue<ConfigValue> &fec_args)
{
    if(!checkArgs("FEC", fec_args.size(), 2))
        return;

    int fec_id;
    string fec_ip;

    fec_args >> fec_id >> fec_ip;

    PRadGEMFEC *new_fec = new PRadGEMFEC(fec_id, fec_ip);
    // failed to register
    if(!Register(new_fec)) {
        delete new_fec;
        return;
    }

    fec_list.push_back(new_fec);
}

void PRadGEMSystem::buildAPV(queue<ConfigValue> &apv_args)
{
    if(!checkArgs("APV", apv_args.size(), 11))
        return;

    string det_name, plane, status;
    int fec_id, adc_ch, orient, index, ts;
    unsigned short hl;
    float cth, zth;

    apv_args >> fec_id >> adc_ch >> det_name >> plane >> orient >> index >> hl
             >> status >> ts >> cth >> zth;

    PRadGEMFEC *fec = GetFEC(fec_id);
    if(fec == nullptr) {// did not find detector
        cout << "PRad GEM System Warning: Failed to add APV " << adc_ch
             << " to FEC " << fec_id
             << ", please check if the FEC exists in GEM configuration file."
             << endl;
        return;
    }

    PRadGEMAPV *new_apv = new PRadGEMAPV(orient, hl, status, ts, cth, zth);
    if(!fec->AddAPV(new_apv, adc_ch)) { // failed to add APV to FEC
        delete new_apv;
        return;
    }

    // trying to connect to Plane
    PRadGEMDetector *det = GetDetector(det_name);
    if(det == nullptr) {
        cout << "PRad GEM System Warning: Cannot find detector " << det_name
             << ", APV " << fec_id << ", " << adc_ch
             << " is not connected to the detector plane. "
             << endl;
        return;
    }

    PRadGEMPlane *pln = det->GetPlane(plane);
    if(pln == nullptr) {
        cout << "PRad GEM System Warning: Cannot find plane " << plane
             << " in detector " << det_name
             << ", APV " << fec_id << ", " << adc_ch
             << " is not connected to the detector plane. "
             << endl;
        return;
    }

    pln->ConnectAPV(new_apv, index);
}

