//============================================================================//
// PRad HyCal System, it contains both HyCal detector and its DAQ system      //
// The connections between HyCal and DAQ system are managed by the system     //
//                                                                            //
// Chao Peng                                                                  //
// 11/12/2016                                                                 //
//============================================================================//

#include "PRadHyCalSystem.h"
#include <iostream>
#include <iomanip>



//============================================================================//
// Constructor, Destructor, Assignment Operators                              //
//============================================================================//

// constructor
PRadHyCalSystem::PRadHyCalSystem(const std::string &path)
: hycal(new PRadHyCalDetector("HyCal", this))
{
    if(!path.empty())
        Configure(path);

    // reserve enough buckets for the adc maps
    adc_addr_map.reserve(ADC_BUCKETS);
    adc_name_map.reserve(ADC_BUCKETS);
}

// destructor
PRadHyCalSystem::~PRadHyCalSystem()
{
    delete hycal;
    ClearADCChannel();
    ClearTDCChannel();
}



//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

// configure HyCal System
void PRadHyCalSystem::Configure(const std::string &path)
{
    ConfigObject::Configure(path);

    if(hycal) {
        hycal->ReadModuleList(GetConfig<std::string>("Module List"));
        hycal->ReadCalibrationFile(GetConfig<std::string>("Calibration File"));
    }

    ReadChannelList(GetConfig<std::string>("DAQ Channel List"));
    ReadPedestalFile(GetConfig<std::string>("DAQ Pedestal File"));
    ReadRunInfoFile(GetConfig<std::string>("Run Info File"));

    BuildConnections();
}

// read DAQ channel list
void PRadHyCalSystem::ReadChannelList(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    // set special splitter
    c_parser.SetSplitters(",: \t");
    if(!c_parser.ReadFile(path)) {
        std::cerr << "PRad HyCal System Error: Failed to read channel list file "
                  << "\"" << path << "\"."
                  << std::endl;
        return;
    }

    // we accept 2 types of channels
    // tdc, adc
    std::vector<std::string> types = {"TDC", "ADC"};
    // tdc args: name crate slot channel
    // adc args: name crate slot channel tdc
    std::vector<int> expect_args = {4, 4};
    std::vector<int> option_args = {0, 1};

    // this vector is to store all the following arguments
    std::vector<std::vector<ConfigValue>> ch_args[types.size()];

    // read all the elements in
    while(c_parser.ParseLine())
    {
        std::string type = c_parser.TakeFirst();
        size_t i = 0;
        for(; i < types.size(); ++i)
        {
            if(ConfigParser::strcmp_case_insensitive(type, types.at(i))) {
                // only save elements from expected format
                if(c_parser.CheckElements(expect_args.at(i), option_args.at(i)))
                    ch_args[i].push_back(c_parser.TakeAll<std::vector>());
                break;
            }
        }

        if(i >= types.size()) { // did not find any type
            std::cout << "PRad HyCal System Warning: Undefined channel type "
                      << type << " in channel list file "
                      << "\"" << path << "\""
                      << std::endl;
        }
    }

    // create TDC first, since it will be needed by ADC channels
    for(auto &args : ch_args[0])
    {
        std::string name(args[0]);
        ChannelAddress addr(args[1].UInt(), args[2].UInt(), args[3].UInt());
        PRadTDCChannel *new_tdc = new PRadTDCChannel(name, addr);
        if(!AddTDCChannel(new_tdc)) // failed to add tdc
            delete new_tdc;
    }

    // create ADC channels, and add them to TDC groups
    for(auto &args : ch_args[1])
    {
        std::string name(args[0]);
        ChannelAddress addr(args[1].UInt(), args[2].UInt(), args[3].UInt());
        PRadADCChannel *new_adc = new PRadADCChannel(name, addr);
        if(!AddADCChannel(new_adc)) { // failed to add adc
            delete new_adc;
            continue;
        }

        // no tdc group specified
        if(args.size() < 5)
            continue;

        // add this adc to tdc group
        std::string tdc_name(args[4]);
        // this adc has no tdc connection
        if(ConfigParser::strcmp_case_insensitive(tdc_name, "NONE") ||
           ConfigParser::strcmp_case_insensitive(tdc_name, "N/A"))
            continue;

        PRadTDCChannel *tdc = GetTDCChannel(tdc_name);
        if(tdc) {
            tdc->ConnectChannel(new_adc);
        } else {
            std::cout << "PRad HyCal System Warning: ADC Channel " << name
                      << " belongs to TDC Group " << tdc_name
                      << ", but the TDC Channel does not exist in the system."
                      << std::endl;
        }
    }
}

// read pedestal file for the adc channels
void PRadHyCalSystem::ReadPedestalFile(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    if(!c_parser.ReadFile(path)) {
        std::cerr << "PRad HyCal System Error: Failed to read pedestal file "
                  << "\"" << path << "\"."
                  << std::endl;
        return;
    }

    double mean, sigma;
    ChannelAddress addr;
    PRadADCChannel *tmp;

    while(c_parser.ParseLine())
    {
        if(!c_parser.CheckElements(5))
            continue;

        c_parser >> addr.crate >> addr.slot >> addr.channel >> mean >> sigma;
        tmp = GetADCChannel(addr);

        if(tmp) {
            tmp->SetPedestal(mean, sigma);
        } else {
            std::cout << "PRad HyCal System Warning: Cannot find ADC Channel "
                      << addr << ", skipped its update for pedestal."
                      << std::endl;
        }
    }

};

// build connections between ADC channels and HyCal modules
void PRadHyCalSystem::BuildConnections()
{
    if(!hycal) {
        std::cout << "PRad HyCal System Warning: HyCal detector does not exist "
                  << "in the system, abort building connections between ADCs "
                  << "and modules"
                  << std::endl;
        return;
    }

    // connect based on name
    for(auto &module : hycal->GetModuleList())
    {
        PRadADCChannel *adc = GetADCChannel(module->GetName());

        if(!adc) { // did not find adc
            std::cout << "PRad HyCal System Warning: Module "
                      << module->GetName()
                      << " has no corresponding ADC channel in the system."
                      << std::endl;
            continue;
        }

        adc->SetModule(module);
        module->SetChannel(adc);
    }
}

// read module status file
void PRadHyCalSystem::ReadRunInfoFile(const std::string &path)
{
    if(path.empty())
        return;

    ConfigParser c_parser;
    if(!c_parser.ReadFile(path)) {
        std::cerr << "PRad HyCal System Error: Failed to read status file "
                  << "\"" << path << "\""
                  << std::endl;
        return;
    }

    std::string name;
    std::vector<double> ref_gain;
    unsigned int ref = 0;

    // first line will be gains for 3 reference PMTs
    if(c_parser.ParseLine()) {
        c_parser >> name;

        if(!ConfigParser::strcmp_case_insensitive(name, "REF_GAIN")) {
            std::cerr << "PRad HyCal System Error: Expected Reference PMT info "
                      << "(started by REF_GAIN) as the first input. Aborted status "
                      << "file reading from "
                      << "\"" << path << "\""
                      << std::endl;
            return;
        }

        // fill in reference PMT gains
        while(c_parser.NbofElements() > 1)
            ref_gain.push_back(c_parser.TakeFirst().Double());

        // get suggested reference number
        c_parser >> ref;
        ref--;

        if(ref >= ref_gain.size()) {
            std::cerr << "PRad HyCal System Error: Unknown Reference PMT "
                      << ref + 1
                      << ", only has " << ref_gain.size()
                      << " Ref. PMTs"
                      << std::endl;
            return;
        }
    }

    double lms_mean, lms_sig, ped_mean, ped_sig;
    unsigned int status;
    // following lines will be information about modules
    while(c_parser.ParseLine())
    {
        if(!c_parser.CheckElements(6))
            continue;

        c_parser >> name >> ped_mean >> ped_sig >> lms_mean >> lms_sig >> status;

        PRadADCChannel *tmp = GetADCChannel(name);
        if(tmp) {
            tmp->SetPedestal(ped_mean, ped_sig);
            tmp->SetDead(status&1);
            PRadHyCalModule *module = tmp->GetModule();
            if(module)
                module->GainCorrection((lms_mean - ped_mean)/ref_gain[ref], ref);
        } else {
            std::cout << "PRad HyCal System Warning: Cannot find ADC Channel "
                      << name << ", skipped status update and gain correction."
                      << std::endl;
        }
    }
}

// add detector, remove the original detector
void PRadHyCalSystem::SetDetector(PRadHyCalDetector *h)
{
    hycal = h;

    if(hycal)
        hycal->SetSystem(this);
}

// remove current detector
void PRadHyCalSystem::RemoveDetector()
{
    if(hycal) {
        hycal->UnsetSystem(true);
        delete hycal, hycal = nullptr;
    }
}

void PRadHyCalSystem::DisconnectDetector(bool force_disconn)
{
    if(hycal) {
        if(!force_disconn)
            hycal->UnsetSystem(true);
        hycal = nullptr;
    }
}

// add adc channel
bool PRadHyCalSystem::AddADCChannel(PRadADCChannel *adc)
{
    if(!adc)
        return false;

    if(GetADCChannel(adc->GetAddress())) {
        std::cerr << "PRad HyCal System Error: Failed to add ADC channel "
                  << adc->GetAddress() << ", a channel with the same address exists."
                  << std::endl;
        return false;
    }

    if(GetADCChannel(adc->GetName())) {
        std::cerr << "PRad HyCal System Error: Failed to add ADC channel "
                  << adc->GetName() << ", a channel with the same name exists."
                  << std::endl;
        return false;
    }

    adc->SetID(adc_list.size());
    adc_list.push_back(adc);
    adc_name_map[adc->GetName()] = adc;
    adc_addr_map[adc->GetAddress()] = adc;
    return true;
}

// add tdc channel
bool PRadHyCalSystem::AddTDCChannel(PRadTDCChannel *tdc)
{
    if(!tdc)
        return false;

    if(GetTDCChannel(tdc->GetAddress())) {
        std::cerr << "PRad HyCal System Error: Failed to add TDC channel "
                  << tdc->GetAddress() << ", a channel with the same address exists."
                  << std::endl;
        return false;
    }

    if(GetTDCChannel(tdc->GetName())) {
        std::cerr << "PRad HyCal System Error: Failed to add ADC channel "
                  << tdc->GetName() << ", a channel with the same name exists."
                  << std::endl;
        return false;
    }

    tdc->SetID(tdc_list.size());
    tdc_list.push_back(tdc);
    tdc_name_map[tdc->GetName()] = tdc;
    tdc_addr_map[tdc->GetAddress()] = tdc;
    return true;
}

void PRadHyCalSystem::ClearADCChannel()
{
    for(auto &adc : adc_list)
        delete adc;
    adc_list.clear();
    adc_name_map.clear();
    adc_addr_map.clear();
}

void PRadHyCalSystem::ClearTDCChannel()
{
    for(auto &tdc : tdc_list)
        delete tdc;
    tdc_list.clear();
    tdc_name_map.clear();
    tdc_addr_map.clear();
}

PRadHyCalModule *PRadHyCalSystem::GetModule(const int &id)
const
{
    if(hycal)
        return hycal->GetModule(id);
    return nullptr;
}

PRadHyCalModule *PRadHyCalSystem::GetModule(const std::string &name)
const
{
    if(hycal)
        return hycal->GetModule(name);
    return nullptr;
}

std::vector<PRadHyCalModule*> PRadHyCalSystem::GetModuleList()
const
{
    if(hycal)
        return hycal->GetModuleList();

    return std::vector<PRadHyCalModule*>();
}

PRadADCChannel *PRadHyCalSystem::GetADCChannel(const int &id)
const
{
    if((size_t)id >= adc_list.size())
        return nullptr;
    return adc_list.at(id);
}

PRadADCChannel *PRadHyCalSystem::GetADCChannel(const std::string &name)
const
{
    auto it = adc_name_map.find(name);
    if(it != adc_name_map.end())
        return it->second;
    return nullptr;
}

PRadADCChannel *PRadHyCalSystem::GetADCChannel(const ChannelAddress &addr)
const
{
    auto it = adc_addr_map.find(addr);
    if(it != adc_addr_map.end())
        return it->second;
    return nullptr;
}

PRadTDCChannel *PRadHyCalSystem::GetTDCChannel(const int &id)
const
{
    if((size_t)id >= tdc_list.size())
        return nullptr;
    return tdc_list.at(id);
}

PRadTDCChannel *PRadHyCalSystem::GetTDCChannel(const std::string &name)
const
{
    auto it = tdc_name_map.find(name);
    if(it != tdc_name_map.end())
        return it->second;
    return nullptr;
}

PRadTDCChannel *PRadHyCalSystem::GetTDCChannel(const ChannelAddress &addr)
const
{
    auto it = tdc_addr_map.find(addr);
    if(it != tdc_addr_map.end())
        return it->second;
    return nullptr;
}
