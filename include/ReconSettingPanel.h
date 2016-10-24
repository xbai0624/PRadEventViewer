#ifndef RECON_SETTING_PANEL_H
#define RECON_SETTING_PANEL_H

#include <QDialog>
#include <string>
#include "PRadCoordSystem.h"

#define COORD_ITEMS 6
#define MATCH_ITEMS 6

class PRadDataHandler;
class PRadDetMatch;

class QLabel;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QDialogButtonBox;

class ReconSettingPanel : public QDialog
{
    Q_OBJECT

public:
    ReconSettingPanel(QWidget *parent = 0);
    ~ReconSettingPanel() {};
    void ConnectDataHandler(PRadDataHandler *h);
    void ConnectCoordSystem(PRadCoordSystem *c);
    void ConnectMatchSystem(PRadDetMatch *m);
    void SyncSettings();
    void SaveSettings();
    void RestoreSettings();
    void ApplyChanges();
    bool ShowHyCalCluster();
    bool ShowGEMCluster();
    bool ShowMatchedHyCal();
    bool ShowMatchedGEM();

private:
    QGroupBox *createHyCalGroup();
    QGroupBox *createGEMGroup();
    QGroupBox *createCoordGroup();
    QGroupBox *createMatchGroup();
    QDialogButtonBox *createStandardButtons();

private slots:
    void updateHyCalPath();
    void loadHyCalConfig();
    void openHyCalConfig();
    void selectCoordData(int r);
    void changeCoordType(int t);
    void saveCoordData();
    void saveCoordFile();
    void openCoordFile();
    void restoreCoordData();

private:
    PRadDataHandler *handler;
    PRadCoordSystem *coordSystem;
    PRadDetMatch *detMatch;

    QGroupBox *hyCalGroup;
    QGroupBox *gemGroup;

    QComboBox *hyCalMethods;
    QLineEdit *hyCalConfigPath;

    QSpinBox *gemMinHits;
    QSpinBox *gemMaxHits;
    QDoubleSpinBox *gemSplitThres;

    QComboBox *coordRun;
    QComboBox *coordType;
    QDoubleSpinBox *coordBox[COORD_ITEMS]; // X, Y, Z, thetaX, thetaY, thetaZ

    QCheckBox *matchHyCal;
    QCheckBox *matchGEM;
    QLabel *matchConfLabel[MATCH_ITEMS];
    QDoubleSpinBox *matchConfBox[MATCH_ITEMS]; // resolution and matching factors

private:
    bool hyCalGroup_data;
    bool gemGroup_data;
    bool matchHyCal_data;
    bool matchGEM_data;
    std::vector<PRadCoordSystem::DetCoord> det_coords;
};

#endif
