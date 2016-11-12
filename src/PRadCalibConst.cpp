//============================================================================//
// Calibration constant class                                                 //
//                                                                            //
// Chao Peng                                                                  //
// 11/12/2016                                                                 //
//============================================================================//

#include "PRadCalibConst.h"



PRadCalibConst::PRadCalibConst(int ref_num)
: factor(0.), base_factor(0.), base_energy(0.), non_linear(0.)
{
    base_gains.resize(ref_num, 0);
}

PRadCalibConst::PRadCalibConst(double f, double e, double nl, double *g, int num)
: factor(f), base_factor(f), base_energy(e), non_linear(nl)
{
    for(int i = 0; i < num; ++i)
        base_gains.push_back(g[i]);
}

PRadCalibConst::~PRadCalibConst()
{
    // place holder
}

void PRadCalibConst::SetRefGain(double gain, int ref)
{
    if((size_t)ref >= base_gains.size()) {
        std::cerr << "PRadCalibConst Error: Cannot update gain for reference PMT "
                  << ref + 1
                  << ", only has "
                  << base_gains.size()
                  << " reference PMTs"
                  << std::endl;
        return;
    }

    base_gains[ref] = gain;
}

void PRadCalibConst::ClearRefGains()
{
    for(auto &gain : base_gains)
        gain = 0;
}

double PRadCalibConst::GetRefGain(int ref)
const
{
    if((size_t)ref >= base_gains.size())
        return 0.;

    return base_gains.at(ref);
}

void PRadCalibConst::GainCorrection(double gain, int ref)
{
    double base = GetRefGain(ref);

    if((gain > 0.) && (base > 0.)) {
        factor = base_factor * base/gain;
    }
}

ConfigParser &operator >>(ConfigParser &p, PRadCalibConst &c)
{
    double f, e, gains[DEFAULT_REF_NUM], nl;

    p >> f >> e;
    for(int i = 0; i < DEFAULT_REF_NUM; ++i)
        p >> gains[i];
    p >> nl;

    return p;
}
