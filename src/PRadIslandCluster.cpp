//============================================================================//
// A C++ wrapper for island reconstruction method                             //
//                                                                            //
// Weizhi Xiong, Chao Peng                                                    //
// 09/28/2016                                                                 //
//============================================================================//
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "PRadIslandCluster.h"

PRadIslandCluster::PRadIslandCluster(PRadDataHandler *h)
: PRadHyCalCluster(h)
{
    fZ0[0] = 2.67; fZ0[1] = 0.86;
    fE0[0] = 2.84e-3; fE0[1] = 1.1e-3;
    UpdateModuleInfo();
}

void PRadIslandCluster::SetHandler(PRadDataHandler *h)
{
    PRadHyCalCluster::SetHandler(h);
    UpdateModuleInfo();
}

//________________________________________________________________
void PRadIslandCluster::Configure(const std::string &c_path)
{
    // if no configuration file specified, load the default value quietly
    bool verbose = false;

    if(!c_path.empty()) {
        readConfigFile(c_path);
        verbose = true;
    }

    // set all the parameters
    fDoShowerDepth = getConfig<int>("DO_SHOWER_DEPTH", 0, verbose);
    fUse2ExpWeight = getConfig<int>("USE_DOUBLE_EXP_WEIGHT", 0, verbose);
    fDoNonLinCorr  = getConfig<int>("DO_NON_LINEARITY_CORRECTION", 0, verbose);
    fMinHitE       = getConfig<float>("MIN_BLOCK_ENERGY", 0.005, verbose);
    // default value is 3.6, suggested by the study with GEM (Weizhi)
    fWeightFreePar = getConfig<float>("WEIGHT_FREE_PAR", 3.6, verbose);
    fMinClusterE   = getConfig<float>("MIN_CLUSTER_E", 0.05, verbose);
    fMinCenterE    = getConfig<float>("MIN_CENTER_E", 0.01, verbose);
    fZLGToPWO      = getConfig<float>("Z_LG_TO_PWO", -10.12, verbose);
//    fZHyCal        = getConfig<float>("Z_HYCAL", 581.7, verbose);
    fCutOffThr     = getConfig<float>("CUT_OFF_THRESHOLD", 0.01, verbose);
    f2ExpFreeWeight= getConfig<float>("DOUBLE_EXP_FREE_WEIGHT", 0.4, verbose);

    // load block info, cluster profile in the PrimEx format
    std::string path;
    path = getConfig<std::string>("CRYSTAL_PROFILE", "config/prof_pwo.dat", verbose);
    LoadCrystalProfile(path);

    path = getConfig<std::string>("LEADGLASS_PROFILE", "config/prof_lg.dat", verbose);
    LoadLeadGlassProfile(path);
}
//________________________________________________________________
void PRadIslandCluster::LoadCrystalProfile(const std::string &path)
{
    char c_path[256];
    strcpy(c_path, path.c_str());
    load_pwo_prof_(c_path, strlen(c_path));
}
//________________________________________________________________
void PRadIslandCluster::LoadLeadGlassProfile(const std::string &path)
{
    char c_path[256];
    strcpy(c_path, path.c_str());
    load_lg_prof_(c_path, strlen(c_path));
}
//FIXME
// non-linearity is part of calibration, should not be part of the clustering
/*
void PRadIslandCluster::LoadNonLinearity(const std::string &path)
{
    FILE *fp;
    fp = fopen(path.c_str(), "r");
    for(int i=0; i < T_BLOCKS; ++i) {
        fscanf(fp,"%f",&(fNonLin[i]));
    }
    fclose(fp);
}
*/
//________________________________________________________________
void PRadIslandCluster::UpdateModuleInfo()
{
    // cannot update
    if(fHandler == nullptr)
        return;

    // initialize block info
    for(int i = 0; i < T_BLOCKS; ++i)
    {
        fBlockINFO[i].id = i+1;
        fBlockINFO[i].x = 0;
        fBlockINFO[i].y = 0;
        fBlockINFO[i].sector = -1;
        fBlockINFO[i].row = 0;
        fBlockINFO[i].col = 0;
    }

    // initialize module status table
    for(int k = 0; k <= 4; ++k)
        for(int i = 0; i < MCOL; ++i)
            for(int j = 0; j < MROW; ++j)
                fModuleStatus[k][i][j] = -1;

    // update block info from module list
    for(auto &module : fHandler->GetChannelList())
    {
        if(!module->IsHyCalModule())
            continue;

        int id = module->GetPrimexID() - 1;
        fBlockINFO[id].x = module->GetX()/10.;
        fBlockINFO[id].y = module->GetY()/10.;

        int sector = module->GetGeometry().sector;
        int row = module->GetGeometry().row;
        int column = module->GetGeometry().column;

        fBlockINFO[id].sector = sector;
        fBlockINFO[id].row = row;
        fBlockINFO[id].col = column;
        if(module->IsDead()) {
            fModuleStatus[sector][column-1][row-1] = 1;
            fDeadModules.push_back(fBlockINFO[id]);
        } else {
            fModuleStatus[sector][column-1][row-1] = 0;
        }
    }
}

//______________________________________________________________
void PRadIslandCluster::Clear()
{
    fNHyCalClusters = 0;
    fNClusterBlocks = 0;
}
//_______________________________________________________________
void PRadIslandCluster::Reconstruct(EventData &event)
{
    //clear and get ready for the new event
    Clear();

    //main function of the hycal reconstruction

    //first load data to the hit array and ech array
    //the second array is used in the fortran island code

    //if not physics event, don't reconstruct it
    if(!event.is_physics_event())
        return;

    LoadModuleData(event);

    //call island reconstruction of each sectors
    //HyCal has 5 sectors, 4 for lead glass one for crystal

    for(int i = 0; i < MSECT; ++i)
    {
        CallIsland(i);
    }

    //glue clusters at the transition region
    GlueTransitionClusters();

    ClusterProcessing();

    FinalProcessing();
}
//_______________________________________________________________
void PRadIslandCluster::LoadModuleData(EventData& event)
{
    //load data to hycalhit array and ech array
    fHandler->ChooseEvent(event);
    std::vector<PRadDAQUnit*> moduleList = fHandler->GetChannelList();

    for(unsigned int i = 0; i < moduleList.size(); ++i)
    {
        PRadDAQUnit* thisModule = moduleList.at(i);
        if(!thisModule->IsHyCalModule())
            continue;
        if(thisModule->GetEnergy()*1e-3 < fMinHitE)
            continue;

        fClusterBlock[fNClusterBlocks].e = thisModule->GetEnergy()*1e-3; //to GeV
        fClusterBlock[fNClusterBlocks].id = thisModule->GetPrimexID();
        fNClusterBlocks++;
    }
}
//________________________________________________________________
void PRadIslandCluster::CallIsland(int isect)
{
    //float xsize, ysize;
    SET_EMIN  = fMinClusterE;   // banks->CONFIG->config->CLUSTER_ENERGY_MIN;
    SET_EMAX  = 9.9;    // banks->CONFIG->config->CLUSTER_ENERGY_MAX;
    SET_HMIN  = 1;      // banks->CONFIG->config->CLUSTER_MIN_HITS_NUMBER;
    SET_MINM  = 0.01;   // banks->CONFIG->config->CLUSTER_MAX_CELL_MIN_ENERGY;
//    ZHYCAL    = fZHyCal;   // ALignment[1].al.z;
    ISECT     = isect;

    for(int icol = 1; icol <= MCOL; ++icol)
    {
        for(int irow = 1; irow <= MROW; ++irow)
        {
            ECH(icol,irow) = 0;
            STAT_CH(icol,irow) = fModuleStatus[isect][icol-1][irow-1];
        }
    }

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

    for(int i = 0; i < fNClusterBlocks; ++i)
    {
        int id  = fClusterBlock[i].id;
        if(fBlockINFO[id-1].sector != isect)
            continue;
        float e = fClusterBlock[i].e;

        if(e < fMinHitE)
            continue;

        int column, row;
        if(id > 1000) {
            column = (id-1001)%NCOL+1;
            row    = (id-1001)/NROW+1;
        } else {
            column = (id-1)%(NCOL+NROW)+1-coloffset;
            row    = (id-1)/(NCOL+NROW)+1-rowoffset;
        }

        ECH(column,row) = int(e*1.e4+0.5);
        ich[column-1][row-1] = id;
    }

    main_island_();

    for(int k = 0; k < adcgam_cbk_.nadcgam; ++k)
    {
        int n = fNHyCalClusters;

        if(fNHyCalClusters == MAX_HCLUSTERS)
        {
            printf("max number of clusters reached\n");
            return;
        }

        float e     = adcgam_cbk_.u.fadcgam[k][0];
//        float x     = adcgam_cbk_.u.fadcgam[k][1];
//        float y     = adcgam_cbk_.u.fadcgam[k][2];
        float xc    = adcgam_cbk_.u.fadcgam[k][4];
        float yc    = adcgam_cbk_.u.fadcgam[k][5];

        float chi2  = adcgam_cbk_.u.fadcgam[k][6];
        int  type   = adcgam_cbk_.u.iadcgam[k][7];
        int  dime   = adcgam_cbk_.u.iadcgam[k][8];
        int  status = adcgam_cbk_.u.iadcgam[k][10];
        if(type >= 90)
        {
            printf("island warning: cluster with type 90+ truncated\n");
            continue;
        }
        if(dime >= MAX_CC)
        {
            printf("island warning: cluster with nhits %i truncated\n",dime);
            continue;
        }

        fHyCalCluster[n].flag     = 0;
        fHyCalCluster[n].det_id   = PRadDetector::HyCal;
        fHyCalCluster[n].type     = type;
        fHyCalCluster[n].nblocks  = dime;
        fHyCalCluster[n].E        = e;
//        fHyCalCluster[n].x        = x;        // biased, unaligned
//        fHyCalCluster[n].y        = y;
        fHyCalCluster[n].chi2     = chi2;
        fHyCalCluster[n].status   = status;
        float ecellmax = -1.; int idmax = -1;
        float sW      = 0.0;
        float xpos    = 0.0;
        float ypos    = 0.0;
        float W;

        for(int j = 0; j < (dime>MAX_CC ? MAX_CC : dime); ++j)
        {
            int id = ICL_INDEX(k,j);
            int kx = (id/100), ky = id%100;
            id = ich[kx-1][ky-1];
            fClusterStorage[n].id[j] = id;

            float ecell = 1.e-4*(float)ICL_IENER(k,j);
            float xcell = fBlockINFO[id-1].x;
            float ycell = fBlockINFO[id-1].y;

            if(type%10 == 1 || type%10 == 2) {
                xcell += xc;
                ycell += yc;
            }

            if(ecell > ecellmax) {
                ecellmax = ecell;
                idmax = id;
            }

            if(ecell > 0.) {
                if(!fUse2ExpWeight) {
                    W = fWeightFreePar + log(ecell/e);
                } else {
                    W = GetDoubleExpWeight(ecell, e);
                }
                if(W > 0) {
                    sW += W;
                    xpos += xcell*W;
                    ypos += ycell*W;
                }
            }

            fClusterStorage[n].E[j]  = ecell;
            fClusterStorage[n].x[j]  = xcell;
            fClusterStorage[n].y[j]  = ycell;

        }

        fHyCalCluster[n].sigma_E    = ecellmax;  // use it temproraly

        if(sW) {
            // FIXME looks like there is no special treatment on projecting z to z = 0
            // after shower depth correction, so it is better to leave depth information
            // and let the coordinate system take care of the projection
            // adding shower depth correction here
            //float zk = 1.;
            if(fDoShowerDepth) {
                if(idmax < 999) {
                    fHyCalCluster[n].z = GetShowerDepth(0, e);
                    fHyCalCluster[n].z -= 10.12; // surface diff between PWO and Pb-Glass
                } else {
                    fHyCalCluster[n].z = GetShowerDepth(1, e);
                }
                //zk /= (1. + fHyCalCluster[n].dz/ZHYCAL);
            } else {
                fHyCalCluster[n].z = 0.; // no correction on depth, all at the surface
            }

            fHyCalCluster[n].x = xpos/sW;
            fHyCalCluster[n].y = ypos/sW;
        } else {
            printf("WRN bad cluster log. coord , center id = %i %f\n", idmax, fHyCalCluster[n].E);
            fHyCalCluster[n].x = 0.;
            fHyCalCluster[n].y = 0.;
        }

        fHyCalCluster[n].cid = idmax;

        for(int j = dime; j < MAX_CC; ++j)  // zero the rest
            fClusterStorage[n].id[j] = 0;

        fNHyCalClusters += 1;
    }
}

//_____________________________________________________________________________
void PRadIslandCluster::GlueTransitionClusters()
{

    // find adjacent clusters and glue
    int ngroup = 0, group_number[MAX_HCLUSTERS], group_size[MAX_HCLUSTERS],
        group_member_list[MAX_HCLUSTERS][MAX_HCLUSTERS];

    for(int i = 0; i < MAX_HCLUSTERS; ++i)
        group_number[i] = 0;

    for(int i = 0; i < fNHyCalClusters; ++i)
    {
        int idi = fHyCalCluster[i].cid;
        //float xi = fBlockINFO[idi-1].x;
        //float yi = fBlockINFO[idi-1].y;

        for(int j = i+1; j < fNHyCalClusters; ++j)
        {
            int idj = fHyCalCluster[j].cid;
            if(fBlockINFO[idi-1].sector == fBlockINFO[idj-1].sector)
                 continue;
            //float xj = fBlockINFO[idj-1].x;
            //float yj = fBlockINFO[idj-1].y;

            if(ClustersMinDist(i,j))
                continue;

            int igr = group_number[i];
            int jgr = group_number[j];

            // create new group and put theese two clusters into it
            if(!igr && !jgr)
            {
                ++ngroup;
                group_number[i] = ngroup; group_number[j] = ngroup;
                group_member_list[ngroup][0] = i;
                group_member_list[ngroup][1] = j;
                group_size[ngroup] = 2;
                continue;
            }
            // add jth cluster into group which ith cluster belongs to
            if(igr && !jgr)
            {
                group_number[j] = igr;
                group_member_list[igr][group_size[igr]] = j;
                group_size[igr] += 1;
                continue;
            }
            // add ith cluster into group which jth cluster belongs to
            if(!igr && jgr)
            {
                group_number[i] = jgr;
                group_member_list[jgr][group_size[jgr]] = i;
                group_size[jgr] += 1;
                continue;
            }
            // add jth group to ith than discard
            if(igr && jgr)
            {
                if(igr == jgr)
                    continue; // nothing to do

                int ni = group_size[igr];
                int nj = group_size[jgr];

                for(int k = 0; k < nj; ++k)
                {
                    group_member_list[igr][ni+k] = group_member_list[jgr][k];
                    group_number[group_member_list[jgr][k]] = igr;
                }

                group_size[igr] += nj;
                group_size[jgr]  = 0;
                continue;
            }
        }
    }

    for(int igr = 1; igr <= ngroup; ++igr)
    {
        if(group_size[igr]<=0)
            continue;
        // sort in increasing cluster number order:

        int list[MAX_HCLUSTERS];
        for(int i = 0; i < group_size[igr]; ++i)
            list[i] = i;

        for(int i = 0; i < group_size[igr]; ++i)
            for(int j = i+1; j < group_size[igr]; ++j)
                if(group_member_list[igr][list[j]] < group_member_list[igr][list[i]])
                {
                    int l   = list[i];
                    list[i] = list[j];
                    list[j] = l;
                }

        int i0 = group_member_list[igr][list[0]];
        if(fHyCalCluster[i0].status == -1)
            continue;

        for(int k = 1; k < group_size[igr]; ++k)
        {
            int k0 = group_member_list[igr][list[k]];
            if(fHyCalCluster[k0].status == -1)
            {
                printf("glue island warning neg. status\n");
                continue;
            }

            MergeClusters(i0, k0);
            fHyCalCluster[k0].status = -1;
        }
    }

    // apply energy nonlin corr.:
    for(int i = 0; i < fNHyCalClusters; ++i)
    {
        float olde = fHyCalCluster[i].E;
        int central_id = fHyCalCluster[i].cid;
        fHyCalCluster[i].E = EnergyCorrect(olde, central_id);
    }

    int ifdiscarded;

    // discrard clusters merged with others:
    do
    {
        ifdiscarded = 0;
        for(int i = 0; i < fNHyCalClusters; ++i)
        {
            if(fHyCalCluster[i].status == -1 ||
               fHyCalCluster[i].E       < SET_EMIN ||
               fHyCalCluster[i].E       > SET_EMAX ||
               fHyCalCluster[i].nblocks < SET_HMIN ||
               fHyCalCluster[i].sigma_E < SET_MINM)
            {
                fNHyCalClusters -= 1;
                int nrest = fNHyCalClusters - i;

                if(nrest)
                {
                    memmove((&fHyCalCluster[0]+i), (&fHyCalCluster[0]+i+1), nrest*sizeof(HyCalHit));
                    memmove((&fClusterStorage[0]+i), (&fClusterStorage[0]+i+1), nrest*sizeof(cluster_t));
                    ifdiscarded = 1;
                }
            }
        }
    } while(ifdiscarded);

}
//________________________________________________________________________
int  PRadIslandCluster::ClustersMinDist(int i,int j)
{
    // min distance between two clusters cells cut
    double mindist = 1.e6, mindx = 1.e6, mindy = 1.e6, dx, dy, sx1, sy1, sx2, sy2;
    int dime1 = fHyCalCluster[i].nblocks, dime2 = fHyCalCluster[j].nblocks;

    for(int k1 = 0; k1 < (dime1>MAX_CC ? MAX_CC : dime1); ++k1)
    {
        if(fClusterStorage[i].E[k1] <= 0.)
            continue;
        if(fBlockINFO[fClusterStorage[i].id[k1]-1].sector) {
            sx1 = sy1 = GLASS_SIZE;
        } else {
            sx1 = CRYS_SIZE_X; sy1 = CRYS_SIZE_Y;
        }

        for(int k2 = 0; k2 < (dime2>MAX_CC ? MAX_CC : dime2); ++k2)
        {
            if(fClusterStorage[j].E[k2] <= 0.)
                continue;
            if(fBlockINFO[fClusterStorage[j].id[k2]-1].sector) {
                sx2 = sy2 = GLASS_SIZE;
            } else {
                sx2 = CRYS_SIZE_X; sy2 = CRYS_SIZE_Y;
            }

            dx = fClusterStorage[i].x[k1] - fClusterStorage[j].x[k2];
            dx = 0.5*dx*(1./sx1+1./sx2);
            dy = fClusterStorage[i].y[k1] - fClusterStorage[j].y[k2];
            dy = 0.5*dy*(1./sy1+1./sy2);

            if(dx*dx+dy*dy<mindist)
            {
                mindist = dx*dx+dy*dy;
                mindx = fabs(dx);
                mindy = fabs(dy);
            }
        }
    }

    return (mindx>1.1 || mindy>1.1);
}
//____________________________________________________________________________
void PRadIslandCluster::MergeClusters(int i, int j)
{
    // ith cluster gona stay; jth cluster gona be discarded
    // leave cluster with greater energy deposition in central cell, discard the second one:
    int i1 = (fHyCalCluster[i].sigma_E > fHyCalCluster[j].sigma_E) ? i : j;
    int dime1 = fHyCalCluster[i].nblocks, dime2 = fHyCalCluster[j].nblocks;
    //int type1 = fHyCalCluster[i].type, type2 = fHyCalCluster[j].type;
    float e = fHyCalCluster[i].E + fHyCalCluster[j].E;

    fHyCalCluster[i].cid     = fHyCalCluster[i1].cid;
    fHyCalCluster[i].sigma_E = fHyCalCluster[i1].sigma_E;
//    fHyCalCluster[i].x       = fHyCalCluster[i1].x;
//    fHyCalCluster[i].y       = fHyCalCluster[i1].y;
    fHyCalCluster[i].chi2    = (fHyCalCluster[i].chi2*dime1 + fHyCalCluster[j].chi2*dime2)/(dime1+dime2);

    if(fHyCalCluster[i].status >= 2)
        fHyCalCluster[i].status += 1; // glued

    fHyCalCluster[i].nblocks = dime1+dime2;
    fHyCalCluster[i].E       = e;
    fHyCalCluster[i].type    = fHyCalCluster[i1].type;

    for(int k1 = 0; k1 < (dime1>MAX_CC ? MAX_CC : dime1); ++k1)
    {
        for(int k2 = 0; k2 < (dime2>MAX_CC ? MAX_CC : dime2); ++k2)
        {
            if(fClusterStorage[i].id[k1] == fClusterStorage[j].id[k2])
            {
                if(fClusterStorage[j].E[k2] > 0.)
                {
                    fClusterStorage[i].E[k1] += fClusterStorage[j].E[k2];
                    fClusterStorage[i].x[k1] += fClusterStorage[j].x[k2];
                    fClusterStorage[i].x[k1] *= 0.5;
                    fClusterStorage[i].y[k1] += fClusterStorage[j].y[k2];
                    fClusterStorage[i].y[k1] *= 0.5;

                    fClusterStorage[j].E[k2] = 0.;
                    fClusterStorage[j].x[k2] = 0.;
                    fClusterStorage[j].y[k2] = 0.;
                }
                continue;
            }
        }
    }

    float sW      = 0.0;
    float xpos    = 0.0;
    float ypos    = 0.0;
    float W;

    for(int k1 = 0; k1 < (dime1>MAX_CC ? MAX_CC : dime1); ++k1)
    {
        float ecell = fClusterStorage[i].E[k1];
        float xcell = fClusterStorage[i].x[k1];
        float ycell = fClusterStorage[i].y[k1];
        if(ecell > 0.)
        {
            W  = fWeightFreePar + log(ecell/e);
            if(W > 0)
            {
                sW += W;
                xpos += xcell*W;
                ypos += ycell*W;
            }
        }
    }

    for(int k2 = 0; k2 < (dime2>MAX_CC ? MAX_CC : dime2); ++k2)
    {
        float ecell = fClusterStorage[j].E[k2];
        float xcell = fClusterStorage[j].x[k2];
        float ycell = fClusterStorage[j].y[k2];

        if(ecell > 0.)
        {
            W  = fWeightFreePar + log(ecell/e);
            if(W > 0)
            {
                sW += W;
                xpos += xcell*W;
                ypos += ycell*W;
            }
        }
    }

    if(sW) {
//        float dx = fHyCalCluster[i1].x_log;
//        float dy = fHyCalCluster[i1].y_log;
        fHyCalCluster[i].x = xpos/sW;
        fHyCalCluster[i].y = ypos/sW;
//        dx = fHyCalCluster[i1].x_log - dx;
//        dy = fHyCalCluster[i1].y_log - dy;
//        fHyCalCluster[i].x += dx;      // shift x0,y0 for glued cluster by x1,y1 shift values
//        fHyCalCluster[i].y += dy;
    } else {
        printf("WRN bad cluster log. coord\n");
        fHyCalCluster[i].x = 0.;
        fHyCalCluster[i].y = 0.;
    }

    // update cluster_storage
    int kk = dime1;

    for(int k = 0; k < (dime2>MAX_CC-dime1 ? MAX_CC-dime1 : dime2); ++k)
    {
        if(fClusterStorage[j].E[k] <= 0.)
            continue;
        if(kk >= MAX_CC)
        {
            printf("WARINING: number of cells in cluster exceeded max %i\n", dime1+dime2);
            break;
        }
        fClusterStorage[i].id[kk] = fClusterStorage[j].id[k];
        fClusterStorage[i].E[kk]  = fClusterStorage[j].E[k];
        fClusterStorage[i].x[kk]  = fClusterStorage[j].x[k];
        fClusterStorage[i].y[kk]  = fClusterStorage[j].y[k];
        fClusterStorage[j].id[k]  = 0;
        fClusterStorage[j].E[k]   = 0.;
        ++kk;
    }

    fHyCalCluster[i].nblocks = kk;
}
//____________________________________________________________________________
float PRadIslandCluster::EnergyCorrect (float c_energy, int central_id)
{
    float energy = c_energy;
    if (fDoNonLinCorr){
        float ecorr = 1. + fNonLin[central_id-1]*(energy-0.55);
        if(fabs(ecorr-1.) < 0.6) energy /= ecorr;
    }
    return energy;
}
//____________________________________________________________________________
void PRadIslandCluster::ClusterProcessing()
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
void PRadIslandCluster::FinalProcessing()
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
inline float PRadIslandCluster::GetShowerDepth(int type, float & E)
{
    if (type >= 2){
        std::cout << "PRadIslandCluster::GetShowerDepth: unexpected type "
                  << type
                  << std::endl;
        return 0;
    }

    return (E > 0.) ? fZ0[type]*log(1.+E/fE0[type]) : 0.;
}
//_________________________________________________________________________________
float PRadIslandCluster::GetDoubleExpWeight(float& e, float& E)
{
    float y = e/E;
    return std::max(0.,1.-Finv(y) /Finv(fCutOffThr))/(1.-Finv(1.)/Finv(fCutOffThr));
}
//_________________________________________________________________________________
inline float PRadIslandCluster::Finv(float y)
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
inline float PRadIslandCluster::Fx(float& x)
{
    return (1.-f2ExpFreeWeight)*exp(-3.*x) + f2ExpFreeWeight*exp(-2.*x/3.);
}







