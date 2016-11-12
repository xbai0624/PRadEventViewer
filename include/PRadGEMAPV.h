#ifndef PRAD_GEM_APV_H
#define PRAD_GEM_APV_H

#include <vector>
#include <fstream>
#include <iostream>
#include "PRadEventStruct.h"
#include "datastruct.h"

//1 time sample data have 128 channel
#define TIME_SAMPLE_SIZE 128

// 12 words before real time sample data
#define TIME_SAMPLE_DIFF 140 // 12 + TIME_SAMPLE_SIZE

// arbitrary number, additional buffer add on time sample data
// this depends on the configuration in the readout list
#define APV_EXTEND_SIZE 130


class PRadGEMFEC;
class PRadGEMPlane;
class TH1I;

class PRadGEMAPV
{
public:
    struct Pedestal
    {
        float offset;
        float noise;

        // initialize with large noise level so there will be no hits instead
        // of maximum hits when gem is not correctly initialized
        Pedestal() : offset(0.), noise(5000.)
        {};
        Pedestal(const float &o, const float &n)
        : offset(o), noise(n)
        {};
    };

    struct StripNb
    {
        unsigned char local;
        int plane;
    };

public:
    // constrcutor
    PRadGEMAPV(const int &orient,
               const int &header_level,
               const std::string &status,
               const size_t &time_sample = 3,
               const float &common_threshold = 20.,
               const float &zero_threshold = 5.);

    // copy/move constructors
    PRadGEMAPV(const PRadGEMAPV &p);
    PRadGEMAPV(PRadGEMAPV &&p);

    // destructor
    virtual ~PRadGEMAPV();

    // copy/move assignment operators
    PRadGEMAPV &operator =(const PRadGEMAPV &p);
    PRadGEMAPV &operator =(PRadGEMAPV &&p);

    // member functions
    void ClearData();
    void ClearPedestal();
    void CreatePedHist();
    void ReleasePedHist();
    void FillPedHist();
    void FitPedestal();
    void FillRawData(const uint32_t *buf, const size_t &siz);
    void FillZeroSupData(const size_t &ch, const size_t &ts, const unsigned short &val);
    void FillZeroSupData(const size_t &ch, const std::vector<float> &vals);
    void SplitData(const uint32_t &buf, float &word1, float &word2);
    void UpdatePedestal(std::vector<Pedestal> &ped);
    void UpdatePedestal(const Pedestal &ped, const size_t &index);
    void UpdatePedestal(const float &offset, const float &noise, const size_t &index);
    void ZeroSuppression();
    void CommonModeCorrection(float *buf, const size_t &size);
    void CommonModeCorrection_Split(float *buf, const size_t &size);
    void CollectZeroSupHits(std::vector<GEM_Data> &hits);
    void CollectZeroSupHits();
    void ResetHitPos();
    void PrintOutPedestal(std::ofstream &out);
    StripNb MapStrip(int ch);

    // get parameters
    int GetFECID() const {return fec_id;};
    int GetADCChannel() const {return adc_ch;};
    GEMChannelAddress GetAddress() const {return GEMChannelAddress(fec_id, adc_ch);};
    size_t GetNTimeSamples() const {return time_samples;};
    size_t GetTimeSampleSize() const {return TIME_SAMPLE_SIZE;};
    int GetOrientation() const {return orient;};
    int GetPlaneIndex() const {return plane_index;};
    int GetHeaderLevel() const {return header_level;};
    bool GetSplitStatus() const {return split;};
    float GetCommonModeThresLevel() const {return common_thres;};
    float GetZeroSupThresLevel() const {return zerosup_thres;};
    size_t GetBufferSize() const {return buffer_size;};
    int GetLocalStripNb(const size_t &ch) const;
    int GetPlaneStripNb(const size_t &ch) const;
    PRadGEMFEC *GetFEC() const {return fec;};
    PRadGEMPlane *GetPlane() const {return plane;};
    std::vector<TH1I *> GetHistList() const;
    std::vector<Pedestal> GetPedestalList() const;

    // set parameters
    void SetFEC(PRadGEMFEC *f, int adc_ch, bool force_set = false);
    void UnsetFEC(bool force_unset = false);
    void SetDetectorPlane(PRadGEMPlane *p, int pl_idx, bool force_set = false);
    void UnsetDetectorPlane(bool force_unset = false);
    void SetTimeSample(const size_t &t);
    void SetOrientation(const int &o) {orient = o;};
    void SetHeaderLevel(const int &h) {header_level = h;};
    void SetCommonModeThresLevel(const float &t) {common_thres = t;};
    void SetZeroSupThresLevel(const float &t) {zerosup_thres = t;};

private:
    void initialize();
    void getAverage(float &ave, const float *buf, const size_t &set = 0);
    size_t getTimeSampleStart();
    void buildStripMap();

private:
    PRadGEMFEC *fec;
    PRadGEMPlane *plane;
    int fec_id;
    int adc_ch;
    int plane_index;

    size_t time_samples;
    int orient;
    int header_level;
    bool split;
    float common_thres;
    float zerosup_thres;
    size_t buffer_size;
    size_t ts_index;
    float *raw_data;
    Pedestal pedestal[TIME_SAMPLE_SIZE];
    StripNb strip_map[TIME_SAMPLE_SIZE];
    bool hit_pos[TIME_SAMPLE_SIZE];
    TH1I *offset_hist[TIME_SAMPLE_SIZE];
    TH1I *noise_hist[TIME_SAMPLE_SIZE];
};

std::ostream &operator <<(std::ostream &os, const GEMChannelAddress &ad);

#endif
