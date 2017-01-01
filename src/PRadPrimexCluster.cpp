//============================================================================//
// A C++ wrapper for island reconstruction method from PrimEx code            //
//                                                                            //
// Weizhi Xiong, Chao Peng                                                    //
// 09/28/2016                                                                 //
//============================================================================//

#include "PRadPrimexCluster.h"


int __prcl_ich[MROW][MCOL];
#define ICH(M,N) __prcl_ich[N-1][M-1]

PRadPrimexCluster::PRadPrimexCluster(const std::string &path)
{
    // configuration
    Configure(path);
}

PRadPrimexCluster::~PRadPrimexCluster()
{
    // place holder
}

PRadHyCalCluster *PRadPrimexCluster::Clone()
const
{
    return new PRadPrimexCluster(*this);
}

void PRadPrimexCluster::Configure(const std::string &path)
{
    PRadHyCalCluster::Configure(path);

    bool verbose = !path.empty();

    // adj_dist is used for merging clusters separated by sectors
    bool corner = getDefConfig<bool>("Corner Connection", false, verbose);
    if(corner)
        adj_dist = CORNER_ADJACENT;
    else
        adj_dist = SIDE_ADJACENT;

    // set the min module energy for all the module type
    float univ_min_energy = getDefConfig<float>("Min Module Energy", 0., false);
    min_module_energy.resize(PRadHyCalModule::Max_Type, univ_min_energy);
    // update the min module energy if some type is specified
    // the key is "Min Module Energy [typename]"
    for(unsigned int i = 0; i < min_module_energy.size(); ++i)
    {
        // determine key name
        std::string type = PRadHyCalModule::get_module_type_name(i);
        std::string key = "Min Module Energy [" + type + "]";
        auto value = GetConfigValue(key);
        if(!value.IsEmpty())
            min_module_energy[i] = value.Float();
    }

    // pass parameters to fortran code
    SET_EMIN  = 0.01;           // banks->CONFIG->config->CLUSTER_ENERGY_MIN;
    SET_EMAX  = 9.9;            // banks->CONFIG->config->CLUSTER_ENERGY_MAX;
    SET_HMIN  = 1;              // banks->CONFIG->config->CLUSTER_MIN_HITS_NUMBER;
    SET_MINM  = 0.01;           // banks->CONFIG->config->CLUSTER_MAX_CELL_MIN_ENERGY;
}

void PRadPrimexCluster::LoadCrystalProfile(const std::string &path)
{
    if(path.empty())
        return;

    char c_path[path.size()];
    strcpy(c_path, path.c_str());
    load_pwo_prof_(c_path, strlen(c_path));
}

void PRadPrimexCluster::LoadLeadGlassProfile(const std::string &path)
{
    if(path.empty())
        return;

    char c_path[path.size()];
    strcpy(c_path, path.c_str());
    load_lg_prof_(c_path, strlen(c_path));
}

void PRadPrimexCluster::UpdateModuleStatus(const std::vector<PRadHyCalModule*> &mlist)
{
    // initialize module status table
    for(int k = 0; k < MSECT; ++k)
        for(int i = 0; i < MCOL; ++i)
            for(int j = 0; j < MROW; ++j)
                module_status[k][i][j] = -1;

    for(auto &module : mlist)
    {
        int sect = module->GetSectorID();
        int col = module->GetColumn();
        int row = module->GetRow();

        if(TEST_BIT(module->GetLayoutFlag(), kDeadModule))
            module_status[sect][col][row] = 1;
        else
            module_status[sect][col][row] = 0;

    }
}

void PRadPrimexCluster::FormCluster(std::vector<ModuleHit> &hits,
                                    std::vector<ModuleCluster> &clusters)
const
{
    // clear container first
    clusters.clear();

    // build a hit map
    std::map<int, ModuleHit*> hit_map;
    for(auto &hit : hits)
    {
        hit_map[hit.id] = &hit;
    }

    // call island reconstruction of each sectors
    // HyCal has 5 sectors, 4 for lead glass one for crystal
    std::vector<std::vector<ModuleCluster>> sect_clusters;
    sect_clusters.resize(MSECT);
    for(int isect = 0; isect < MSECT; ++isect)
    {
        callIsland(hits, isect);
        sect_clusters[isect] = getIslandResult(hit_map);
    }

    // glue clusters separated by the sector
    glueClusters(clusters, sect_clusters);
}

void PRadPrimexCluster::callIsland(const std::vector<ModuleHit> &hits, int isect)
const
{
    // determine sector's column and row ranges
    ISECT = isect;
    int coloffset = 0, rowoffset = 0;
    switch(isect)
    {
    case 0:
        NCOL = 34; NROW = 34;
        SET_XSIZE = CRYS_SIZE_X; SET_YSIZE = CRYS_SIZE_Y;
        break;
    case 1:
        NCOL = 24; NROW =  6;
        SET_XSIZE = GLASS_SIZE; SET_YSIZE = GLASS_SIZE;
        coloffset = 0, rowoffset = 0;
        break;
    case 2:
        NCOL =  6; NROW =  24;
        SET_XSIZE = GLASS_SIZE; SET_YSIZE = GLASS_SIZE;
        coloffset = 24, rowoffset = 0;
        break;
    case 3:
        NCOL = 24; NROW =  6;
        SET_XSIZE = GLASS_SIZE; SET_YSIZE = GLASS_SIZE;
        coloffset =  6, rowoffset = 24;
        break;
    case 4:
        NCOL =  6; NROW = 24;
        SET_XSIZE = GLASS_SIZE; SET_YSIZE = GLASS_SIZE;
        coloffset = 0, rowoffset = 6;
        break;
    default:
        printf("call_island bad sector given : %i\n",isect);
        exit(1);
    }

    // load module status and reset energies
    for(int icol = 1; icol <= NCOL; ++icol)
    {
        for(int irow = 1; irow <= NROW; ++irow)
        {
            ECH(icol,irow) = 0;
            ICH(icol,irow) = -1;
            STAT_CH(icol,irow) = module_status[isect][icol-1][irow-1];
        }
    }

    // load hits in this sector
    for(auto &hit : hits)
    {
        // not belong to this sector or energy is too low
        if((hit.sector != isect) ||
           (hit.energy < min_module_energy.at(hit.geo.type)))
            continue;

        const int &id  = hit.id;
        int column, row;
        if(id > 1000) {
            column = (id-1001)%NCOL+1;
            row    = (id-1001)/NROW+1;
        } else {
            column = (id-1)%(NCOL+NROW)+1-coloffset;
            row    = (id-1)/(NCOL+NROW)+1-rowoffset;
        }

        // discretize to 0.1 MeV
        ECH(column,row) = int(hit.energy*10. + 0.5);
        ICH(column,row) = id;
    }

    // call fortran code to reconstruct clusters
    main_island_();
}

// get result from fortran island code
std::vector<ModuleCluster> PRadPrimexCluster::getIslandResult(const std::map<int, ModuleHit*> &hmap)
const
{
    std::vector<ModuleCluster> res;
    res.reserve(10);

    for(int k = 0; k < adcgam_cbk_.nadcgam; ++k)
    {
        int dime = adcgam_cbk_.u.iadcgam[k][8];
        ModuleCluster cluster;
        // GeV to MeV
        cluster.energy = adcgam_cbk_.u.fadcgam[k][0]*1000.;
        cluster.leakage = cluster.energy;

        for(int j = 0; j < (dime>MAX_CC ? MAX_CC : dime); ++j)
        {
            int add = ICL_INDEX(k,j);
            int kx = (add/100), ky = add%100;
            int id = ICH(kx, ky);
            // convert back from 0.1 MeV
            float ecell = 0.1*(float)ICL_IENER(k,j);
            cluster.leakage -= ecell;
            auto it = hmap.find(id);
            if(it != hmap.end())
            {
                ModuleHit hit(*it->second);
                hit.energy = ecell;
                cluster.hits.push_back(hit);
            }
        }
        cluster.FindCenter();
        res.push_back(cluster);
    }

    return res;
}

void PRadPrimexCluster::glueClusters(std::vector<ModuleCluster> &clusters,
                                     std::vector<std::vector<ModuleCluster>> &s)
const
{
    // merge clusters between sectors
    for(size_t isect = 0; isect < MSECT; ++isect)
    {
        auto &base = s.at(isect);
        for(int jsect = isect + 1; jsect < MSECT; ++jsect)
        {
            auto &sector = s.at(jsect);
            for(size_t i = 0; i < sector.size(); ++i)
            {
                for(auto &cluster : base)
                {
                    if(checkTransAdj(sector.at(i), cluster)) {
                        cluster.Merge(sector.at(i));
                        sector.erase(sector.begin() + i);
                        i--;
                        break;
                    }
                }
            }
        }
    }

    // add clusters to the container
    for(auto &sect : s)
    {
        for(auto &cluster : sect)
        {
            clusters.emplace_back(std::move(cluster));
        }
    }
}

inline bool PRadPrimexCluster::checkTransAdj(const ModuleCluster &c1,
                                             const ModuleCluster &c2)
const
{
    for(auto &m1 : c1.hits)
    {
        for(auto &m2 : c2.hits)
        {
            if((m1.sector != m2.sector) &&
               (PRadHyCalDetector::hit_distance(m1, m2) < adj_dist)) {
                return true;
            }
        }
    }

    return false;
}

void PRadPrimexCluster::LeakCorr(ModuleCluster &, const std::vector<ModuleHit> &)
const
{
    // place holder
    // TODO island.F has corrected the leakage
    // here we probably can add some inforamtion about the leakage correction
}
/*
void PRadPrimexCluster::ClusterProcessing()
{
    //float pi = 3.1415926535;
    //  final cluster processing (add status and energy resolution sigma_E):
    for(int i = 0; i < fNHyCalClusters; ++i)
    {
        float e   = fHyCalCluster[i].E;
        int idmax = fHyCalCluster[i].cid;

        // apply 1st approx. non-lin. corr. (was obtained for PWO):
        //e *= 1. + 200.*pi/pow((pi+e),7.5);
        //fHyCalCluster[i].E = e;

        //float x   = fHyCalCluster[i].x1;
        //float y   = fHyCalCluster[i].y1;

        //coord_align(i, e, idmax);
        int status = fHyCalCluster[i].status;
        int type   = fHyCalCluster[i].type;
        int dime   = fHyCalCluster[i].nblocks;

        if(status < 0)
        {
            printf("error in island : cluster with negative status");
            exit(1);
        }

        int itp, ist;
        itp  = (idmax>1000) ? 0 : 10;
        if(status==1)
            itp += 1;

        for(int k = 0; k < (dime>MAX_CC ? MAX_CC : dime); ++k)
        {
            if(itp<10) {
                if(fClusterStorage[i].id[k] < 1000)
                {
                    itp =  2;
                    break;
                }
            } else {
                if(fClusterStorage[i].id[k] > 1000)
                {
                    itp = 12;
                    break;
                }
            }
        }

        fHyCalCluster[i].type = itp;

        ist = type;

        if(status > 2)
            ist += 20; // glued
        fHyCalCluster[i].status = ist;

        double se = (idmax>1000) ? sqrt(0.9*0.9*e*e+2.5*2.5*e+1.0) : sqrt(2.3*2.3*e*e+5.4*5.4*e);
        se /= 100.;
        if(itp%10 == 1) {
            se *= 1.5;
        } else if(itp%10 == 2) {
            if(itp>10)
                se *= 0.8;
            else
                se *=1.25;
        }
        fHyCalCluster[i].sigma_E = se;
    }
}
//____________________________________________________________________________
void PRadPrimexCluster::FinalProcessing()
{
    // old PrimEx island output is in GeV and cm
    // the x axis is also reverted comparing to our convention
    // here transform the PrimEx unit convention to ours
    for(int i = 0; i < fNHyCalClusters; ++i)
    {
        if (fHyCalCluster[i].type == 0  || fHyCalCluster[i].type == 1)
        SET_BIT(fHyCalCluster[i].flag, kPWO);
        if (fHyCalCluster[i].type == 10 || fHyCalCluster[i].type == 11)
        SET_BIT(fHyCalCluster[i].flag, kLG);
        if (fHyCalCluster[i].type == 2  || fHyCalCluster[i].type == 12)
        SET_BIT(fHyCalCluster[i].flag, kTransition);
        if (fHyCalCluster[i].type == 1){
            SET_BIT(fHyCalCluster[i].flag, kInnerBound);
            SET_BIT(fHyCalCluster[i].flag, kPWO);
        }
        if (fHyCalCluster[i].type == 11){
            SET_BIT(fHyCalCluster[i].flag, kOuterBound);
            SET_BIT(fHyCalCluster[i].flag, kLG);
        }
        if (fHyCalCluster[i].status == 10 || fHyCalCluster[i].status == 30)
        SET_BIT(fHyCalCluster[i].flag, kSplit);

        for (unsigned int j = 0; j < fDeadModules.size(); j++){
            float r = sqrt( pow((fDeadModules.at(j).x - fHyCalCluster[i].x), 2) +
                            pow((fDeadModules.at(j).y - fHyCalCluster[i].y), 2) );
            float size = 0;
            if (fDeadModules.at(j).sector == 0) size = CRYS_SIZE_X;
            else size = GLASS_SIZE;

            if (r/size < 1.5) SET_BIT(fHyCalCluster[i].flag, kDeadModule);
        }


        // x axis is flipped due to the difference between PrimEx coordinates and
        // current PRad coordinates
        fHyCalCluster[i].E *= 1000.; // GeV to MeV
        fHyCalCluster[i].sigma_E *= 1000.; // GeV to MeV
        fHyCalCluster[i].x *= 10.; // cm to mm
        fHyCalCluster[i].y *= 10.; // cm to mm
        fHyCalCluster[i].z *= 10.; //cm to mm

//        fHyCalCluster[i].x_log *= -10.; // cm to mm
//        fHyCalCluster[i].y_log *= 10.; // cm to mm


        PRadDAQUnit *module =
        fHandler->GetChannel(PRadDAQUnit::NameFromPrimExID(fHyCalCluster[i].cid));

        PRadTDCGroup *tdc = module->GetTDCGroup();
        if(tdc)
            fHyCalCluster[i].set_time(tdc->GetTimeMeasure());
        else
            fHyCalCluster[i].clear_time();

    }
}
//_______________________________________________________________________________
float PRadPrimexCluster::GetDoubleExpWeight(float& e, float& E)
{
    float y = e/E;
    return std::max(0.,1.-Finv(y) /Finv(fCutOffThr))/(1.-Finv(1.)/Finv(fCutOffThr));
}
//_________________________________________________________________________________
inline float PRadPrimexCluster::Finv(float y)
{
    float x0 = 0.;
    float y0 = 0.;
    float x1 = -10.;
    float x2 = 10.;
    float y1 = Fx(x1) - y;
    // FIXME y2 is not used? comment out to avoid warning
    //float y2 = Fx(x2) - y;
    while(fabs(x1-x2) > 1e-6)
    {
        x0 = 0.5*(x1+x2);
        y0 = Fx(x0) - y;
        if(y1*y0 > 0) {
          x1 = x0;
          y1 = y0;
        }else{
          x2 = x0;
          //y2 = y0;
        }
    }
    return x0;
}
//_________________________________________________________________________________
inline float PRadPrimexCluster::Fx(float& x)
{
    return (1.-f2ExpFreeWeight)*exp(-3.*x) + f2ExpFreeWeight*exp(-2.*x/3.);
}
*/
