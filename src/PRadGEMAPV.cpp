//============================================================================//
// GEM APV class                                                              //
// APV is the basic unit for GEM DAQ system, it will be connected to GEM Plane//
//                                                                            //
// Chao Peng                                                                  //
// 10/07/2016                                                                 //
//============================================================================//

#include <iostream>
#include <iomanip>
#include "PRadGEMFEC.h"
#include "PRadGEMPlane.h"
#include "PRadGEMAPV.h"
#include "TF1.h"
#include "TH1.h"



//============================================================================//
// constructor, assigment operator, destructor                                //
//============================================================================//

// constructor
PRadGEMAPV::PRadGEMAPV(const int &o,
                       const int &hl,
                       const std::string &s,
                       const size_t &t,
                       const float &cth,
                       const float &zth)
: orient(o), header_level(hl), common_thres(cth), zerosup_thres(zth)
{
    // initialize
    initialize();

    raw_data = nullptr;
    SetTimeSample(t);

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        offset_hist[i] = nullptr;
        noise_hist[i] = nullptr;
    }

    if(s.find("split") != std::string::npos)
        split = true;
    else
        split = false;

    ClearData();
}

// only used in constructors
// initialize the members that should not be copied
void PRadGEMAPV::initialize()
{
    // these can only be assigned by a FEC (SetFEC)
    fec = nullptr;
    fec_id = -1;
    adc_ch = -1;

    // these can only be assigned by a Plane (SetDetectorPlane)
    plane = nullptr;
    plane_index = -1;
}

// The copy and move constructor/assignment operator won't copy or replace the
// current connection between APV and Plane
// copy constructor
PRadGEMAPV::PRadGEMAPV(const PRadGEMAPV &that)
: time_samples(that.time_samples), orient(that.orient),
  header_level(that.header_level), split(that.split),
  common_thres(that.common_thres), zerosup_thres(that.zerosup_thres)
{
    initialize();

    // raw data related
    buffer_size = that.buffer_size;
    ts_index = that.ts_index;
    // dangerous part, may fail due to lack of memory
    raw_data = new float[buffer_size];
    // copy values
    for(size_t i = 0; i < buffer_size; ++i)
    {
        raw_data[i] = that.raw_data[i];
    }

    // copy other arrays
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        pedestal[i] = that.pedestal[i];
        strip_map[i] = that.strip_map[i];
        hit_pos[i] = that.hit_pos[i];

        // dangerous part, may fail due to lack of memory
        if(that.offset_hist[i] != nullptr) {
            offset_hist[i] = new TH1I(*that.offset_hist[i]);
        } else {
            offset_hist[i] = nullptr;
        }

        if(that.noise_hist[i] != nullptr) {
            noise_hist[i] = new TH1I(*that.noise_hist[i]);
        } else {
            noise_hist[i] = nullptr;
        }
    }
}

// move constructor
PRadGEMAPV::PRadGEMAPV(PRadGEMAPV &&that)
: time_samples(that.time_samples), orient(that.orient),
  header_level(that.header_level), split(that.split),
  common_thres(that.common_thres), zerosup_thres(that.zerosup_thres)
{
    initialize();

    // raw_data related
    buffer_size = that.buffer_size;
    ts_index = that.ts_index;
    raw_data = that.raw_data;
    // null the pointer of that
    that.buffer_size = 0;
    that.raw_data = nullptr;

    // other arrays
    // static array, so no need to move, just copy elements
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        pedestal[i] = that.pedestal[i];
        strip_map[i] = that.strip_map[i];
        hit_pos[i] = that.hit_pos[i];

        // these need to be moved
        offset_hist[i] = that.offset_hist[i];
        noise_hist[i] = that.noise_hist[i];
        that.offset_hist[i] = nullptr;
        that.noise_hist[i] = nullptr;
    }
}

// destructor
PRadGEMAPV::~PRadGEMAPV()
{
    UnsetFEC();
    UnsetDetectorPlane();
    ReleasePedHist();

    delete[] raw_data;
}

// copy assignment operator
PRadGEMAPV &PRadGEMAPV::operator= (const PRadGEMAPV &rhs)
{
    PRadGEMAPV apv(rhs); // use copy constructor
    *this = std::move(apv); // use move assignment operator
    return *this;
}

// move assignment operator
PRadGEMAPV &PRadGEMAPV::operator= (PRadGEMAPV &&rhs)
{
    // release memory
    ReleasePedHist();
    delete[] raw_data;

    // members
    time_samples = rhs.time_samples;
    orient = rhs.orient;
    header_level = rhs.header_level;
    split = rhs.split;
    common_thres = rhs.common_thres;
    zerosup_thres = rhs.zerosup_thres;

    // raw_data related
    buffer_size = rhs.buffer_size;
    ts_index = rhs.ts_index;
    raw_data = rhs.raw_data;
    // null the pointer of that
    rhs.buffer_size = 0;
    rhs.raw_data = nullptr;

    // other arrays
    // static array, so no need to move, just copy elements
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        pedestal[i] = rhs.pedestal[i];
        strip_map[i] = rhs.strip_map[i];
        hit_pos[i] = rhs.hit_pos[i];

        // these need to be moved
        offset_hist[i] = rhs.offset_hist[i];
        noise_hist[i] = rhs.noise_hist[i];
        rhs.offset_hist[i] = nullptr;
        rhs.noise_hist[i] = nullptr;
    }

    return *this;
}



//============================================================================//
// Public Member Functions                                                    //
//============================================================================//

// connect the apv to GEM FEC
void PRadGEMAPV::SetFEC(PRadGEMFEC *f, int slot, bool force_set)
{
    if(f == fec && slot == adc_ch)
        return;

    if(!force_set)
        UnsetFEC();

    if(f) {
        fec = f;
        fec_id = fec->GetID();
        adc_ch = slot;
    }
}

// disconnect the fec, reset fec id and adc ch
void PRadGEMAPV::UnsetFEC(bool force_unset)
{
    if(!fec)
        return;

    if(!force_unset)
        fec->DisconnectAPV(adc_ch, true);

    fec = nullptr;
    fec_id = -1;
    adc_ch = -1;
}

// connect the apv to GEM Plane
void PRadGEMAPV::SetDetectorPlane(PRadGEMPlane *p, int slot, bool force_set)
{
    if(p == plane && slot == plane_index)
        return;

    if(!force_set)
        UnsetDetectorPlane();

    if(p) {
        plane = p;
        plane_index = slot;
        // strip map is related to plane that connected, thus build the map
        buildStripMap();
    }
}

// disconnect the plane, reset plane index
void PRadGEMAPV::UnsetDetectorPlane(bool force_unset)
{
    if(!plane)
        return;

    if(!force_unset)
        plane->DisconnectAPV(plane_index, true);

    plane = nullptr;
    plane_index = -1;
}

// create histograms
void PRadGEMAPV::CreatePedHist()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(offset_hist[i] == nullptr) {
            std::string name = "CH_" + std::to_string(i) + "_OFFSET_"
                             + std::to_string(fec_id) + "_"
                             + std::to_string(adc_ch);
            offset_hist[i] = new TH1I(name.c_str(), "Pedestal", 500, 2000, 3500);
        }
        if(noise_hist[i] == nullptr) {
            std::string name = "CH_" + std::to_string(i) + "_NOISE_"
                             + std::to_string(fec_id) + "_"
                             + std::to_string(adc_ch);
            noise_hist[i] = new TH1I(name.c_str(), "Noise", 400, -200, 200);
        }
    }
}

void PRadGEMAPV::ResetPedHist()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(offset_hist[i])
            offset_hist[i]->Reset();
        if(noise_hist[i])
            noise_hist[i]->Reset();
    }
}

// release the memory for histograms
void PRadGEMAPV::ReleasePedHist()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        delete offset_hist[i], offset_hist[i] = nullptr;
        delete noise_hist[i], noise_hist[i] = nullptr;
    }
}

// set time samples and reserve memory for raw data
void PRadGEMAPV::SetTimeSample(const size_t &t)
{
    time_samples = t;
    buffer_size = t*TIME_SAMPLE_DIFF + APV_EXTEND_SIZE;

    // reallocate the memory for proper size
    delete[] raw_data;

    raw_data = new float[buffer_size];

    ClearData();
}

// clear all the data
void PRadGEMAPV::ClearData()
{
    // set to a high value that won't trigger zero suppression
    for(size_t i = 0; i < buffer_size; ++i)
        raw_data[i] = 5000.;

    ResetHitPos();
}

// reset hit position array
void PRadGEMAPV::ResetHitPos()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
        hit_pos[i] = false;
}

// clear all the pedestal
void PRadGEMAPV::ClearPedestal()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
        pedestal[i] = Pedestal(0, 0);
}

// update pedestal
void PRadGEMAPV::UpdatePedestal(std::vector<Pedestal> &ped)
{
    for(size_t i = 0; (i < ped.size()) && (i < TIME_SAMPLE_SIZE); ++i)
        pedestal[i] = ped[i];
}

// update single channel pedestal
void PRadGEMAPV::UpdatePedestal(const Pedestal &ped, const size_t &index)
{
    if(index >= TIME_SAMPLE_SIZE)
        return;

    pedestal[index] = ped;
}

// update single channel pedestal
void PRadGEMAPV::UpdatePedestal(const float &offset, const float &noise, const size_t &index)
{
    if(index >= TIME_SAMPLE_SIZE)
        return;

    pedestal[index].offset = offset;
    pedestal[index].noise = noise;
}

// fill raw data
void PRadGEMAPV::FillRawData(const uint32_t *buf, const size_t &size)
{
    if(2*size > buffer_size) {
        std::cerr << "Received " << size * 2 << " adc words, "
                  << "but APV " << adc_ch << " in FEC " << fec_id
                  << " has only " << buffer_size << " channels" << std::endl;
        return;
    }

    for(size_t i = 0; i < size; ++i)
    {
        SplitData(buf[i], raw_data[2*i], raw_data[2*i+1]);
    }

    ts_index = getTimeSampleStart();
}

// fill zero suppressed data
void PRadGEMAPV::FillZeroSupData(const size_t &ch, const size_t &ts, const unsigned short &val)
{
    ts_index = 0;
    size_t idx = ch + ts_index + ts*TIME_SAMPLE_DIFF;
    if(ts >= time_samples ||
       ch >= TIME_SAMPLE_SIZE ||
       idx >= buffer_size)
    {
        std::cerr << "GEM APV Error: Failed to fill zero suppressed data, "
                  << " channel " << ch << " or time sample " << ts
                  << " is not allowed."
                  << std::endl;
        return;
    }

    hit_pos[ch] = true;
    raw_data[idx] = val;
}

// fill zero suppressed data
void PRadGEMAPV::FillZeroSupData(const size_t &ch, const std::vector<float> &vals)
{
    ts_index = 0;

    if(vals.size() != time_samples || ch >= TIME_SAMPLE_SIZE)
    {
        std::cerr << "GEM APV Error: Failed to fill zero suppressed data, "
                  << " channel " << ch << " or time sample " << vals.size()
                  << " is not allowed."
                  << std::endl;
        return;
    }

    hit_pos[ch] = true;

    for(size_t i = 0; i < vals.size(); ++i)
    {
        size_t idx = ch + ts_index + i*TIME_SAMPLE_DIFF;
        raw_data[idx] = vals[i];
    }

}

// split the data word, since one data word stores two channels' data
void PRadGEMAPV::SplitData(const uint32_t &data, float &word1, float &word2)
{
    int data1 = (((data>>16)&0xff)<<8) | (data>>24);
    int data2 = ((data&0xff)<<8) | ((data>>8)&0xff);
    word1 = (float)data1;
    word2 = (float)data2;
}

// fill pedestal histogram
void PRadGEMAPV::FillPedHist()
{
    float average[2][3];

    for(size_t i = 0; i < time_samples; ++i)
    {
        if(split) {
            getAverage(average[0][i], &raw_data[ts_index + i*TIME_SAMPLE_DIFF], 1);
            getAverage(average[1][i], &raw_data[ts_index + i*TIME_SAMPLE_DIFF], 2);
        } else {
            getAverage(average[0][i], &raw_data[ts_index + i*TIME_SAMPLE_DIFF]);
        }
    }

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        float ch_average = 0.;
        float noise_average = 0.;
        for(size_t j = 0; j < time_samples; ++j)
        {
            ch_average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF];
            if(split) {
                if(strip_map[i].local < 16)
                    noise_average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF] - average[0][j];
                else
                    noise_average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF] - average[1][j];
            } else {
                noise_average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF] - average[0][j];
            }
        }

        if(offset_hist[i])
            offset_hist[i]->Fill(ch_average/time_samples);

        if(noise_hist[i])
            noise_hist[i]->Fill(noise_average/time_samples);
    }
}

// fit pedestal histogram
void PRadGEMAPV::FitPedestal()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if( (offset_hist[i] == nullptr) ||
            (noise_hist[i] == nullptr) ||
            (offset_hist[i]->Integral() < 1000) ||
            (noise_hist[i]->Integral() < 1000) )
            continue;

        offset_hist[i]->Fit("gaus", "qww");
        noise_hist[i]->Fit("gaus", "qww");
        TF1 *myfit = (TF1*) offset_hist[i]->GetFunction("gaus");
        double p0 = myfit->GetParameter(1);
        myfit = (TF1*) noise_hist[i]->GetFunction("gaus");
        double p1 = myfit->GetParameter(2);
        UpdatePedestal((float)p0, (float)p1, i);
    }
}

// do zero suppression in raw data space
void PRadGEMAPV::ZeroSuppression()
{
    if(plane == nullptr)
    {
        std::cerr << "GEM APV Error: APV "
                  << fec_id << ", " << adc_ch
                  << " is not connected to a detector plane, "
                  << "cannot handle data without correct mapping."
                  << std::endl;
        return;
    }

    if((ts_index + TIME_SAMPLE_DIFF*(time_samples - 1) + TIME_SAMPLE_SIZE) >= buffer_size)
    {
        std::cout << fec_id << ", " << adc_ch << "  "
                  << "incorrect time sample position: "  << ts_index
                  << " " << buffer_size << " " << time_samples
                  << std::endl;
        return;
    }

    // common mode correction
    for(size_t ts = 0; ts < time_samples; ++ts)
    {
        if(split)
            CommonModeCorrection_Split(&raw_data[ts_index + ts*TIME_SAMPLE_DIFF], TIME_SAMPLE_SIZE);
        else
            CommonModeCorrection(&raw_data[ts_index + ts*TIME_SAMPLE_DIFF], TIME_SAMPLE_SIZE);
    }

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        float average = 0.;
        for(size_t j = 0; j < time_samples; ++j)
        {
            average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF];
        }
        average /= time_samples;

        if(average > pedestal[i].noise * zerosup_thres)
            hit_pos[i] = true;
        else
            hit_pos[i] = false;
    }
}

// collect zero suppressed hit in raw data space, need a container input
void PRadGEMAPV::CollectZeroSupHits(std::vector<GEM_Data> &hits)
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(hit_pos[i] == false)
            continue;

        GEM_Data hit(fec_id, adc_ch, i);
        for(size_t j = 0; j < time_samples; ++j)
        {
            hit.values.emplace_back(raw_data[i + ts_index + j*TIME_SAMPLE_DIFF]);
        }
        hits.emplace_back(hit);
    }
}

// collect zero suppressed hit in raw data space, directly to connected Plane
void PRadGEMAPV::CollectZeroSupHits()
{
    if(plane == nullptr)
        return;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(hit_pos[i] == false)
            continue;

        std::vector<float> charges;
        for(size_t j = 0; j < time_samples; ++j)
        {
            charges.push_back(raw_data[i + ts_index + j*TIME_SAMPLE_DIFF]);
        }
        plane->AddStripHit(strip_map[i].plane, charges);
    }
}

// do common mode correction (bring the signal average to 0)
void PRadGEMAPV::CommonModeCorrection(float *buf, const size_t &size)
{
    int count = 0;
    float average = 0;

    for(size_t i = 0; i < size; ++i)
    {
        buf[i] = pedestal[i].offset - buf[i];

        if(buf[i] < pedestal[i].noise * common_thres) {
            average += buf[i];
            count++;
        }
    }

    if(count)
        average /= (float)count;

    for(size_t i = 0; i < size; ++i)
    {
        buf[i] -= average;
    }
}

// do common mode correction for split APV
void PRadGEMAPV::CommonModeCorrection_Split(float *buf, const size_t &size)
{
    int count1 = 0, count2 = 0;
    float average1 = 0, average2 = 0;

    for(size_t i = 0; i < size; ++i)
    {
        buf[i] = pedestal[i].offset - buf[i];
        if(strip_map[i].local < 16) {
            if(buf[i] < pedestal[i].noise * common_thres * 10.) {
                average1 += buf[i];
                count1++;
            }
        } else {
            if(buf[i] < pedestal[i].noise * common_thres) {
                average2 += buf[i];
                count2++;
            }
        }
    }

    if(count1)
        average1 /= (float)count1;
    if(count2)
        average2 /= (float)count2;

    for(size_t i = 0; i < size; ++i)
    {
        if(strip_map[i].local < 16)
            buf[i] -= average1;
        else
            buf[i] -= average2;
    }
}

// get the local strip number from strip map
int PRadGEMAPV::GetLocalStripNb(const size_t &ch)
const
{
   if(ch >= TIME_SAMPLE_SIZE) {
       std::cerr << "GEM APV Get Local Strip Error:"
                 << " APV " << adc_ch
                 << " in FEC " << fec_id
                 << " only has " << TIME_SAMPLE_SIZE
                 << " channels." << std::endl;
       return -1;
   }

   return (int)strip_map[ch].local;
}

// get the plane strip number from strip map
int PRadGEMAPV::GetPlaneStripNb(const size_t &ch)
const
{
   if(ch >= TIME_SAMPLE_SIZE) {
       std::cerr << "GEM APV Get Plane Strip Error:"
                 << " APV " << adc_ch
                 << " in FEC " << fec_id
                 << " only has " << TIME_SAMPLE_SIZE
                 << " channels." << std::endl;
       return -1;
   }

   return strip_map[ch].plane;
}

// mapping the channel to strip number
PRadGEMAPV::StripNb PRadGEMAPV::MapStrip(int ch)
{
    StripNb result;

    // calculate local strip mapping
    // APV25 Internal Channel Mapping
    int strip = 32*(ch%4) + 8*(ch/4) - 31*(ch/16);

    // APV25 Channel to readout strip Mapping
    if((plane->GetType() == PRadGEMPlane::Plane_X) && (plane_index == 11)) {
        if(strip & 1)
            strip = 48 - (strip + 1)/2;
        else
            strip = 48 + strip/2;
    } else {
        if(strip & 1)
            strip = 32 - (strip + 1)/2;
        else
            strip = 32 + strip/2;
    }

    strip &= 0x7f;
    result.local = strip;

    // calculate plane strip mapping
    // reverse strip number by orient
    if(orient != plane->GetOrientation())
        strip = 127 - strip;

    // special APV
    if((plane->GetType() == PRadGEMPlane::Plane_X) && (plane_index == 11)) {
        strip += -16 + TIME_SAMPLE_SIZE * (plane_index - 1);
    } else {
        strip += TIME_SAMPLE_SIZE * plane_index;
    }

    result.plane = strip;

    return result;
}

// print the pedestal information to ofstream
void PRadGEMAPV::PrintOutPedestal(std::ofstream &out)
{
    out << "APV "
        << std::setw(12) << fec_id
        << std::setw(12) << adc_ch
        << std::endl;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        out << std::setw(12) << i
            << std::setw(12) << pedestal[i].offset
            << std::setw(12) << pedestal[i].noise
            << std::endl;
    }
}

// return all the existing histograms
std::vector<TH1I *> PRadGEMAPV::GetHistList()
const
{
    std::vector<TH1I *> hist_list;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(offset_hist[i])
            hist_list.push_back(offset_hist[i]);
        if(noise_hist[i])
            hist_list.push_back(noise_hist[i]);
    }

    return hist_list;
}

// pack all pedestal info into a vector and return
std::vector<PRadGEMAPV::Pedestal> PRadGEMAPV::GetPedestalList()
const
{
    std::vector<Pedestal> ped_list;
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        ped_list.push_back(pedestal[i]);
    }

    return ped_list;
}

//============================================================================//
// Private Member Functions                                                   //
//============================================================================//

// Compare the data with header level and find where the time sample data begin
size_t PRadGEMAPV::getTimeSampleStart()
{
    for(size_t i = 2; i < buffer_size; ++i)
    {
        if( (raw_data[i]   < header_level) &&
            (raw_data[i-1] < header_level) &&
            (raw_data[i-2] < header_level) )
            return i + 10;
    }

    return buffer_size;
}

// get the average within one time sample
// set is for split apv
// if set = 0, it is normal apv, all strips are in
// if set = 1, it is a splitted apv, the set 1, first 16 strips are in
// if set = 2, it is a splitted apv, the set 2, other strips are in
void PRadGEMAPV::getAverage(float &average, const float *buf, const size_t &set)
{
    average = 0.;
    int count = 0;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if((set == 0) ||
           (set == 1 && strip_map[i].local < 16) ||
           (set == 2 && strip_map[i].local >= 16))
        {
            average += buf[i];
            count++;
        }
    }

    average /= (float)count;
}

// Build strip map
// both local strip map and plane strip map are related to the connected plane
// thus this function will only be called when the APV is connected to the plane
void PRadGEMAPV::buildStripMap()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        strip_map[i] = MapStrip(i);
    }
}

//============================================================================//
// Non-Class-Member Functions                                                 //
//============================================================================//
std::ostream &operator <<(std::ostream &os, const GEMChannelAddress &ad)
{
    return os << ad.fec_id << ", " << ad.adc_ch;
}

