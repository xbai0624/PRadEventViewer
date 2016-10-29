//============================================================================//
// A global list for all the detectors, change name here                      //
//                                                                            //
//============================================================================//

#include "PRadDetectors.h"
#include <iostream>
#include <cstring>

static const char *__detector_list[] = {"HyCal", "PRadGEM1", "PRadGEM2", "Undefined"};

const char *PRadDetectors::getName(int enumVal)
{
    if(enumVal < 0 || enumVal > (int)Max_Dets)
        return "";

    return __detector_list[enumVal];
}

int PRadDetectors::getID(const char *name)
{
    for(int i = 0; i < (int)Max_Dets; ++i)
        if(strcmp(name, __detector_list[i]) == 0)
            return i;

    std::cerr << "PRad Detectors Error: Cannot find " << name
              << ", please check if the detector name exists in the PRadDetectors."
              << std::endl;
    // not found
    return -1;
}
