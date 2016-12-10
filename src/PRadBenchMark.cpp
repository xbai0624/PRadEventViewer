//============================================================================//
// A timer class to test the performance                                      //
//                                                                            //
// Chao Peng                                                                  //
// 02/12/2016                                                                 //
//============================================================================//

#include "PRadBenchMark.h"



PRadBenchMark::PRadBenchMark()
{
    Reset();
}

PRadBenchMark::~PRadBenchMark()
{
    // place holder
}

void PRadBenchMark::Reset()
{
    time_point = std::chrono::high_resolution_clock::now();
}

unsigned int PRadBenchMark::GetElapsedTime()
const
{
    auto time_end = std::chrono::high_resolution_clock::now();
    auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_point);
    return int_ms.count();
}
