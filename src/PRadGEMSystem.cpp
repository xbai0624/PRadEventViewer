//============================================================================//
// GEM System class                                                           //
// It has both the physical and the DAQ structure of GEM detectors in PRad    //
// Physical structure: 2 detectors, each has 2 planes (X, Y)                  //
// DAQ structure: several FECs, each has several APVs                         //
// APVs and detector planes are connected                                     //
//                                                                            //
// FECs and Detectors are directly managed by GEM System                      //
// Each FEC or Detector will manage its own APV or Plane members              //
//                                                                            //
// Framework of the DAQ system (DET-PLN & FEC-APV) are based on the code from //
// Kondo Gnanvo and Xinzhan Bai                                               //
//                                                                            //
// Chao Peng                                                                  //
// 10/29/2016                                                                 //
//============================================================================//

#include "PRadGEMSystem.h"
#include "ConfigParser.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <list>
#include "TFile.h"
#include "TH1.h"

using namespace std;



//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//

// constructor
PRadGEMSystem::PRadGEMSystem(const string &config_file, int daq_cap, int det_cap)
: gem_recon(new PRadGEMCluster()), PedestalMode(false),
  def_ts(3), def_cth(20.), def_zth(5), def_ctth(8)
{
    daq_slots.resize(daq_cap, nullptr);
    det_slots.resize(det_cap, nullptr);

    if(!config_file.empty())
        Configure(config_file);
}

// the copy and move constructor will not only copy all the members that managed
// by GEM System, but also build the connections between them
// copy constructor, the complicated part is to copy the connections between
// Planes and APVs
PRadGEMSystem::PRadGEMSystem(const PRadGEMSystem &that)
: ConfigObject(that),
  gem_recon(new PRadGEMCluster(*that.gem_recon)), PedestalMode(that.PedestalMode),
  def_ts(that.def_ts), def_cth(that.def_cth), def_zth(that.def_zth),
  def_ctth(that.def_ctth)
{
    gem_recon = new PRadGEMCluster(*that.gem_recon);

    // copy daq system first
    for(auto &fec : that.daq_slots)
    {
        if(fec == nullptr)
            daq_slots.push_back(nullptr);
        else
            daq_slots.push_back(new PRadGEMFEC(*fec));
    }

    // then copy detectors and planes
    for(auto &det : that.det_slots)
    {
        if(det == nullptr) {
            det_slots.push_back(nullptr);
        } else {
            PRadGEMDetector *new_det = new PRadGEMDetector(*det);
            det_slots.push_back(new_det);

            // copy the connections between APVs and planes
            auto that_planes = det->GetPlaneList();
            for(uint32_t i = 0; i < that_planes.size(); ++i)
            {
                auto that_apvs = that_planes[i]->GetAPVList();
                PRadGEMPlane *this_plane = new_det->GetPlaneList().at(i);
                for(auto &apv : that_apvs)
                {
                    PRadGEMAPV *this_apv = GetAPV(apv->GetAddress());
                    this_plane->ConnectAPV(this_apv, apv->GetPlaneIndex());
                }
            }
        }
    }
    RebuildDAQMap();
    RebuildDetectorMap();
}

// move constructor
PRadGEMSystem::PRadGEMSystem(PRadGEMSystem &&that)
: ConfigObject(that),
  PedestalMode(that.PedestalMode), det_list(move(that.det_list)),
  fec_list(move(that.fec_list)), daq_slots(move(that.daq_slots)),
  det_slots(move(that.det_slots)), det_name_map(move(that.det_name_map)),
  def_ts(that.def_ts), def_cth(that.def_cth), def_zth(that.def_zth),
  def_ctth(that.def_ctth)
{
    gem_recon = that.gem_recon;
    that.gem_recon = nullptr;

    // reset the system for all components
    for(auto &fec : fec_list)
        fec->SetSystem(this);

    for(auto &det : det_list)
        det->SetSystem(this);
}

// desctructor
PRadGEMSystem::~PRadGEMSystem()
{
    Clear();
    delete gem_recon;
}

// copy assignment operator
PRadGEMSystem &PRadGEMSystem::operator =(const PRadGEMSystem &rhs)
{
    PRadGEMSystem that(rhs); // use copy constructor
    *this = move(that); // use move assignment operator
    return *this;
}

// move assignment operator
PRadGEMSystem &PRadGEMSystem::operator =(PRadGEMSystem &&rhs)
{
    ConfigObject::operator =(rhs);

    // release current resources
    Clear();
    delete gem_recon;

    // move everything here
    gem_recon = rhs.gem_recon;
    rhs.gem_recon = nullptr;
    PedestalMode = rhs.PedestalMode;
    det_list = move(rhs.det_list);
    fec_list = move(rhs.fec_list);
    daq_slots = move(rhs.daq_slots);
    det_slots = move(rhs.det_slots);
    det_name_map = move(rhs.det_name_map);
    def_ts = rhs.def_ts;
    def_cth = rhs.def_cth;
    def_zth = rhs.def_zth;
    def_ctth = rhs.def_ctth;

    // reset the system for all components
    for(auto &fec : fec_list)
        fec->SetSystem(this);

    for(auto &det : det_list)
        det->SetSystem(this);

    return *this;
}



//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

// configure this class
void PRadGEMSystem::Configure(const string &path)
{
    bool verbose = false;

    if(!path.empty()) {
        ConfigObject::Configure(path);
        verbose = true;
    }

    def_ts = getDefConfig<unsigned int>("Default Time Samples", 3, verbose);
    def_cth = getDefConfig<float>("Default Common Mode Threshold", 20, verbose);
    def_zth = getDefConfig<float>("Default Zero Suppression Threshold", 5, verbose);
    def_ctth = getDefConfig<float>("Default Cross Talk Threshold", 8, verbose);

    if(gem_recon)
        gem_recon->Configure(GetConfig<std::string>("GEM Cluster Configuration"));

    try{
        ReadMapFile(GetConfig<std::string>("GEM Map"));
        ReadPedestalFile(GetConfig<std::string>("GEM Pedestal"));
    } catch(PRadException &e) {
        std::cerr << e.FailureType() << ": "
                  << e.FailureDesc() << std::endl;
    }
}

// remove all the components
void PRadGEMSystem::Clear()
{
    // DAQ removal
    for(auto &fec : daq_slots)
    {
        // this prevent fec from calling RemoveFEC upon destruction
        if(fec)
            fec->UnsetSystem(true);
        delete fec;
    }

    fec_list.clear();

    // Detectors removal
    for(auto &det : det_slots)
    {
        // this prevent detector from calling removeDetector upon destruction
        if(det)
            det->UnsetSystem(true);
        delete det;
    }

    det_list.clear();
    det_name_map.clear();
}

// Read the map file
void PRadGEMSystem::ReadMapFile(const string &path) throw(PRadException)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    c_parser.SetSplitters(",");

    if(!c_parser.ReadFile(path)) {
        throw PRadException("GEM System", "cannot open GEM map file " + path);
    }

    // release memory before load new configuration
    Clear();

    // we accept 4 types of elements
    // detector, plane, fec, apv
    vector<string> types = {"DET", "PLN", "FEC", "APV"};
    vector<int> expect_args = {3, 6, 2, 8};
    vector<int> option_args = {0, 0, 0, 4};
    // this vector is to store all the following arguments
    vector<list<ConfigValue>> args[types.size()];

    // read all the elements in
    while(c_parser.ParseLine())
    {
        string key = c_parser.TakeFirst();
        uint32_t i = 0;
        for(; i < types.size(); ++i)
        {
            if(ConfigParser::strcmp_case_insensitive(key, types.at(i))) {
                if(c_parser.CheckElements(expect_args.at(i), option_args.at(i)))
                    args[i].push_back(c_parser.TakeAll<list>());
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

    // Rebuilding the maps just helps sort the lists, so they won't depend on
    // the orders in configuration map
    RebuildDAQMap();
    RebuildDetectorMap();
}

// Load pedestal file and update all APVs' pedestal
void PRadGEMSystem::ReadPedestalFile(const string &path) throw(PRadException)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    c_parser.SetSplitters(",: \t");

    if(!c_parser.ReadFile(path)) {
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

            if(apv == nullptr) {
                cout << "PRad GEM System Warning: Cannot find APV "
                     << fec <<  ", " << adc
                     << " , skip updating its pedestal."
                     << endl;
            }
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

    if((uint32_t)det->GetDetID() >= det_slots.size())
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

    if((uint32_t)fec->GetID() >= daq_slots.size())
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

// disconnect detector, and rebuild the detector map
void PRadGEMSystem::DisconnectDetector(int det_id, bool force_disconn)
{
    if((uint32_t)det_id >= det_slots.size())
        return;

    auto &det = det_slots[det_id];
    if(!det)
        return;

    if(!force_disconn)
        det->UnsetSystem(true);

    det = nullptr;
    // rebuild maps
    RebuildDetectorMap();
}

// remove detector, and rebuild the detector map
void PRadGEMSystem::RemoveDetector(int det_id)
{
    if((uint32_t)det_id >= det_slots.size())
        return;

    auto &det = det_slots[det_id];
    if(!det)
        return;

    det->UnsetSystem(true);
    delete det, det = nullptr;

    // rebuild maps
    RebuildDetectorMap();
}

// disconnect FEC, and rebuild the DAQ map
void PRadGEMSystem::DisconnectFEC(int fec_id, bool force_disconn)
{
    if((uint32_t)fec_id >= daq_slots.size())
        return;

    auto &fec = daq_slots[fec_id];
    if(!fec)
        return;

    if(!force_disconn)
        fec->UnsetSystem(true);

    fec = nullptr;
    // rebuild maps
    RebuildDAQMap();
}

void PRadGEMSystem::RemoveFEC(int fec_id)
{
    if((uint32_t)fec_id >= daq_slots.size())
        return;

    auto &fec = daq_slots[fec_id];
    if(!fec)
        return;

    fec->UnsetSystem(true);
    delete fec, fec = nullptr;

    // rebuild maps
    RebuildDAQMap();
}

// rebuild detector related maps
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

// rebuild daq related maps
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

// find detector by detector id
PRadGEMDetector *PRadGEMSystem::GetDetector(const int &det_id)
const
{
    if((unsigned int) det_id >= det_slots.size()) {
        return nullptr;
    }
    return det_slots[det_id];
}

// find detector by its name
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

// find FEC by its id
PRadGEMFEC *PRadGEMSystem::GetFEC(const int &id)
const
{
    if((uint32_t)id < daq_slots.size()) {
        return daq_slots[id];
    }

    return nullptr;
}

// find APV by its FEC id and adc channel
PRadGEMAPV *PRadGEMSystem::GetAPV(const int &fec_id, const int &apv_id)
const
{
    if (((uint32_t) fec_id < daq_slots.size()) &&
        (daq_slots.at(fec_id) != nullptr)) {

        return daq_slots.at(fec_id)->GetAPV(apv_id);
    }

    return nullptr;
}

// find APV by its ChannelAddress
PRadGEMAPV *PRadGEMSystem::GetAPV(const GEMChannelAddress &addr)
const
{
    return GetAPV(addr.fec_id, addr.adc_ch);
}

// fill raw data to a certain apv
void PRadGEMSystem::FillRawData(const GEMRawData &raw, EventData &event)
{
    PRadGEMAPV *apv = GetAPV(raw.addr);

    if(apv != nullptr) {

        apv->FillRawData(raw.buf, raw.size);

        if(event.is_monitor_event()) {
            if(PedestalMode)
                apv->FillPedHist();
        } else {
            apv->ZeroSuppression();
#ifdef MULTI_THREAD
            locker.lock();
#endif
            apv->CollectZeroSupHits(event.get_gem_data());
#ifdef MULTI_THREAD
            locker.unlock();
#endif
        }
    }
}

// clear all APVs' raw data space
void PRadGEMSystem::Reset()
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::ClearData);
        fec->APVControl(&PRadGEMAPV::ResetPedHist);
    }

    for(auto &det : det_list)
    {
        det->Reset();
    }
}

// fill zero suppressed data and re-collect these data in GEM_Data format
void PRadGEMSystem::FillZeroSupData(const vector<GEMZeroSupData> &data_pack, EventData &event)
{
    // clear all the APVs' hits
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::ResetHitPos);
    }

    // fill in the online zero-suppressed data
    for(auto &data : data_pack)
        FillZeroSupData(data);

#ifdef MULTI_THREAD
        locker.lock();
#endif
    // collect these zero-suppressed hits
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::CollectZeroSupHits, event.get_gem_data());
    }
#ifdef MULTI_THREAD
        locker.unlock();
#endif
}

// fill zero suppressed data
void PRadGEMSystem::FillZeroSupData(const GEMZeroSupData &data)
{
    PRadGEMAPV *apv = GetAPV(data.addr);

    if(apv != nullptr) {
        apv->FillZeroSupData(data.channel, data.time_sample, data.adc_value);
    }
}

// update EventData to all APVs
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

    for(auto &det : det_list)
    {
        det->CollectHits();
    }
}

// reconstruct certain event
void PRadGEMSystem::Reconstruct(const EventData &data)
{
    // only reconstruct physics event
    if(!data.is_physics_event())
        return;

    ChooseEvent(data);
    Reconstruct();
}

void PRadGEMSystem::Reconstruct()
{
    for(auto &det : det_list)
    {
        det->Reconstruct(gem_recon);
    }
}

// fit pedestal for all APVs
// this requires pedestal mode is on, otherwise there won't be any data to fit
void PRadGEMSystem::FitPedestal()
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::FitPedestal);
    }
}

// save pedestal file for all APVs
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

// set pedestal mode on/off
// if the pedestal mode is on, filling raw data will also fill the histograms in
// APV for future pedestal fitting
// turn on the pedestal mode will greatly slow down the raw data handling and
// consume a significant amount of memories
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

// collect the zero suppressed data from APV
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

// change the common mode threshold level for all APVs
void PRadGEMSystem::SetUnivCommonModeThresLevel(const float &thres)
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::SetCommonModeThresLevel, thres);
    }
}

// change the zero suppression threshold level for all APVs
void PRadGEMSystem::SetUnivZeroSupThresLevel(const float &thres)
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::SetZeroSupThresLevel, thres);
    }
}

// change the time sample for all APVs
void PRadGEMSystem::SetUnivTimeSample(const uint32_t &ts)
{
    for(auto &fec : fec_list)
    {
        fec->APVControl(&PRadGEMAPV::SetTimeSample, ts);
    }
}

// save all APVs' histograms into a root file
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

// get the whole APV list
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

// a helper operator to make arguments reading easier
template<typename T>
list<ConfigValue> &operator >>(list<ConfigValue> &lhs, T &t)
{
    if(lhs.empty()) {
        t = ConfigValue("0").Convert<T>();
    } else {
        t = lhs.front().Convert<T>();
        lhs.pop_front();
    }

    return lhs;
}

// build the detectors according to the arguments
void PRadGEMSystem::buildDetector(list<ConfigValue> &det_args)
{
    string readout, type, name;
    det_args >> readout >> type >> name;
    PRadGEMDetector *new_det = new PRadGEMDetector(readout, type, name);

    if(!Register(new_det)) {
        delete new_det;
        return;
    }

    // successfully registered, build maps
    // the map will be needed for add planes into detectors
    det_list.push_back(new_det);
    det_name_map[name] = new_det;
}

// build the planes according to the arguments
// since planes need to be added into existing detectors, it requires all the
// detectors to be built first and a proper detector map is generated
void PRadGEMSystem::buildPlane(list<ConfigValue> &pln_args)
{
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

// build the FECs according to the arguments
void PRadGEMSystem::buildFEC(list<ConfigValue> &fec_args)
{
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

// build the APVs according to the arguments
// since APVs need to be added into existing FECs, and be connected to existing
// planes, it requires FEC and planes to be built first and proper maps are generated
void PRadGEMSystem::buildAPV(list<ConfigValue> &apv_args)
{
    string det_name, plane, status;
    int fec_id, adc_ch, orient, index;
    unsigned short hl;
    apv_args >> fec_id >> adc_ch >> det_name >> plane >> orient >> index >> hl
             >> status;

    // optional arguments
    unsigned int ts = def_ts;
    float cth = def_cth, zth = def_zth, ctth = def_ctth;
    if(apv_args.size())
        apv_args >> ts;
    if(apv_args.size())
        apv_args >> cth;
    if(apv_args.size())
        apv_args >> zth;
    if(apv_args.size())
        apv_args >> ctth;

    PRadGEMFEC *fec = GetFEC(fec_id);
    if(fec == nullptr) {// did not find detector
        cout << "PRad GEM System Warning: Failed to add APV " << adc_ch
             << " to FEC " << fec_id
             << ", please check if the FEC exists in GEM configuration file."
             << endl;
        return;
    }

    PRadGEMAPV *new_apv = new PRadGEMAPV(orient, hl, status, ts, cth, zth, ctth);
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
