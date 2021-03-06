#ifndef PRAD_PRIMEX_CLUSTER_H
#define PRAD_PRIMEX_CLUSTER_H

#include <map>
#include <string>
#include <vector>
#include "PRadHyCalCluster.h"

//this is a c++ wrapper around the primex island algorithm
//used for HyCal cluster reconstruction

//define some global constants
#define MSECT 5
#define MCOL 34
#define MROW 34

#define CRYSTAL_BLOCKS 1156 // 34x34 array (2x2 hole in the center)
#define GLASS_BLOCKS 900    // 30x30 array holes are also counted (18x18 in the center)
#define BLANK_BLOCKS 100    // For simplifying indexes

#define T_BLOCKS 2156

#define MAX_HHITS 1728 // For Hycal
#define MAX_CC 60

#define nint_phot_cell  5
#define ncoef_phot_cell 3
#define dcorr_phot_cell 16

#define CRYS_HALF_ROWS 17 //CRYS_ROWS/2
#define GLASS_HALF_ROWS 15 //GLASS_ROWS/2
#define CRYS_HALF_SIZE_X 1.0385   // CRYS_SIZE_X/2
#define CRYS_HALF_SIZE_Y 1.0375   // CRYS_SIZE_Y/2
#define GLASS_HALF_SIZE 1.9075    // GLASS_SIZE/2
#define GLASS_OFFSET_X CRYS_SIZE_X*CRYS_ROWS //Distance from center to glass by X axis
#define GLASS_OFFSET_Y CRYS_SIZE_Y*CRYS_ROWS //Distance from center to glass by Y axis

#define CRYS_ROWS 34
#define GLASS_ROWS 30
#define CRYS_SIZE_X 2.077   // real X-size of crystal
#define CRYS_SIZE_Y 2.075   // real Y-size of crystal
#define GLASS_SIZE 3.815    // real size of glass

typedef struct
{
    int id;                 // ID of block
    float x;                // Center of block x-coord
    float y;                // Center of block y-coord
    int sector;             // 0 for W, 1 - 4 for Glass (clockwise starting at noon)
    int row;                // row number within sector
    int col;                // column number within sector
} blockINFO_t;


typedef struct
{
    int  id[MAX_CC];   // ID of ith block, where i runs from 0 to 8
    float E[MAX_CC];   // Energy of ith block
    float x[MAX_CC];   // Center of ith block x-coord
    float y[MAX_CC];   // Center of ith block y-coord
} cluster_t;

typedef struct
{
    int  id;   // ID of ADC
    float e;   // Energy of ADC
} cluster_block_t;

extern "C"
{
    void load_pwo_prof_(char* config_dir, int str_len);
    void load_lg_prof_(char* config_dir, int str_len);
    void main_island_();
    extern struct
    {
        int ech[MROW][MCOL];
    } ech_common_;

    extern struct
    {
        int stat_ch[MROW][MCOL];
    } stat_ch_common_;

    extern struct
    {
        int icl_index[MAX_CC][200], icl_iener[MAX_CC][200];
    } icl_common_;

    extern struct
    {
        float xsize, ysize, mine, maxe;
        int min_dime;
        float minm;
        int ncol, nrow;
        float zhycal;
        int isect;
    } set_common_;

    extern struct
    {
        int nadcgam;
        union
        {
            int iadcgam[50][11];
            float fadcgam[50][11];
        } u;
    } adcgam_cbk_;

    extern struct
    {
        float fa[100];
    } hbk_common_;

    #define ECH(M,N) ech_common_.ech[N-1][M-1]
    #define STAT_CH(M,N) stat_ch_common_.stat_ch[N-1][M-1]
    #define ICL_INDEX(M,N) icl_common_.icl_index[N][M]
    #define ICL_IENER(M,N) icl_common_.icl_iener[N][M]
    #define HEGEN(N) read_mcfile_com_.hegen[N-1]
    #define SET_XSIZE set_common_.xsize
    #define SET_YSIZE set_common_.ysize
    #define SET_EMIN  set_common_.mine
    #define SET_EMAX  set_common_.maxe
    #define SET_HMIN  set_common_.min_dime
    #define SET_MINM  set_common_.minm
    #define NCOL      set_common_.ncol
    #define NROW      set_common_.nrow
    #define ZHYCAL    set_common_.zhycal
    #define ISECT     set_common_.isect
    #define FA(N) hbk_common_.fa[N-1]
}

class PRadPrimexCluster : public PRadHyCalCluster
{
public:
    PRadPrimexCluster(const std::string &path = "");
    virtual ~PRadPrimexCluster();
    PRadHyCalCluster *Clone() const;

    void Configure(const std::string &path);
    void LoadCrystalProfile(const std::string &path);
    void LoadLeadGlassProfile(const std::string &path);
    void UpdateModuleStatus(const std::vector<PRadHyCalModule*> &mlist);
    void FormCluster(std::vector<ModuleHit> &hits,
                     std::vector<ModuleCluster> &clusters) const;
    void LeakCorr(ModuleCluster &c, const std::vector<ModuleHit> &dead) const;

private:
    void callIsland(const std::vector<ModuleHit> &hits, int isect) const;
    std::vector<ModuleCluster> getIslandResult(const std::map<int, ModuleHit*> &hmap) const;
    void glueClusters(std::vector<ModuleCluster> &b, std::vector<ModuleCluster> &s) const;
    bool checkTransAdj(const ModuleCluster &c1, const ModuleCluster &c2) const;

private:
    float adj_dist;
    std::vector<float> min_module_energy;
    int module_status[MSECT][MCOL][MROW];
};

#endif
