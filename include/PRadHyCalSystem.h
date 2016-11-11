#ifndef PRAD_HYCAL_SYSTEM_H
#define PRAD_HYCAL_SYSTEM_H

#include <string>
#include "PRadHyCalDetector.h"
#include "ConfigObject.h"

class PRadHyCalSystem : public ConfigObject
{
public:
    PRadHyCalSystem(const std::string &path);
    virtual ~PRadHyCalSystem();

    void Configure(const std::string &path);
    void ReadModuleList(const std::string &path);
    void AddDetector(PRadHyCalDetector *h);
    void RemoveDetector();

    PRadHyCalDetector *GetDetector() const {return hycal;};

private:
    PRadHyCalDetector *hycal;
};

#endif
