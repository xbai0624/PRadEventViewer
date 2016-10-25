#ifndef PRAD_DETECTORS_H
#define PRAD_DETECTORS_H

// TODO make it a parent class for all detectors
// to achieve this, need to develop a HyCal class first
// now only provides detector lists to connect all the components
namespace PRadDetectors
{
    enum DetectorEnum
    {
        HyCal = 0,
        PRadGEM1,
        PRadGEM2,
        Max_Dets
    };

    const char *getName(int enumVal);
    int getID(const char *);
};

#endif
