//============================================================================//
// Basic PRad Cluster Reconstruction Class For HyCal                          //
// Different reconstruction methods can be implemented accordingly            //
//                                                                            //
// Chao Peng, Weizhi Xiong                                                    //
// 09/28/2016                                                                 //
//============================================================================//

#include <cmath>
#include <iostream>
#include <iomanip>
#include "PRadHyCalCluster.h"

PRadHyCalCluster::PRadHyCalCluster(PRadDataHandler *h)
: fHandler(h), fNHyCalClusters(0)
{
    // place holder
}

PRadHyCalCluster::~PRadHyCalCluster()
{
    // place holder
}

void PRadHyCalCluster::Clear()
{
    fNHyCalClusters = 0;
}

void PRadHyCalCluster::SetHandler(PRadDataHandler *h)
{
    fHandler = h;
}

void PRadHyCalCluster::UpdateModuleInfo()
{
    // to be implemented by methods
}

void PRadHyCalCluster::Reconstruct(EventData & /*event*/)
{
    // to be implemented by methods
}
