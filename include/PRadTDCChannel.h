#ifndef PRAD_TDC_CHANNEL_H
#define PRAD_TDC_CHANNEL_H

#include <vector>
#include <string>
#include <unordered_map>
#include "PRadDAQChannel.h"

class TH1;

class PRadTDCChannel : public PRadDAQChannel
{
public:
    // constructor
    PRadTDCChannel(const std::string &n, const ChannelAddress &addr);

    // copy/move constructors
    PRadTDCChannel(const PRadTDCChannel &that);
    PRadTDCChannel(PRadTDCChannel &&that);

    // destructor
    virtual ~PRadTDCChannel();

    // copy/move assignment operators
    PRadTDCChannel &operator =(const PRadTDCChannel &rhs);
    PRadTDCChannel &operator =(PRadTDCChannel &&rhs);

    void AddChannel(PRadADCChannel *ch);
    void RemoveChannel(int id);
    void AddTimeMeasure(const unsigned short &count);
    void AddTimeMeasure(const std::vector<unsigned short> &counts);
    void SetTimeMeasure(const std::vector<unsigned short> &counts);
    void Reset();
    void ResetHists();
    void ClearTimeMeasure();
    void FillHist(const unsigned short &time);

    PRadADCChannel* GetADCChannel(int id) const;
    TH1 *GetHist() const {return tdc_hist;};
    const std::vector<PRadADCChannel*> &GetGroupList() const;
    const std::vector<unsigned short> &GetTimeMeasure() const {return time_measure;};

private:
    std::unordered_map<int, PRadADCChannel*> group_map;
    std::vector<unsigned short> time_measure;
    TH1 *tdc_hist;
};

#endif
