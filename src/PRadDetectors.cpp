//============================================================================//
// A global list for all the detectors, change name here                      //
//                                                                            //
//============================================================================//

#include "PRadDetectors.h"
#include <iostream>
#include <cstring>

static const char *DetectorList[] = {"HyCal", "PRadGEM1", "PRadGEM2", "Undefined"};

const char *PRadDetectors::getName(int enumVal)
{
    if(enumVal < 0 || enumVal > (int)Max_Dets)
        return "";

    return DetectorList[enumVal];
}

int PRadDetectors::getID(const char *name)
{
    for(int i = 0; i < (int)Max_Dets; ++i)
        if(strcmp(name, DetectorList[i]) == 0)
            return i;

    std::cout << "PRad Detectors : Cannot find " << name
              << ", please check if the detector name exists in the PRadDetectors."
              << " Return HyCal id as default."
              << std::endl;
    // not found
    return HyCal;
}
