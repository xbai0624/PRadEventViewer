#ifndef RECON_SETTING_PANEL_H
#define RECON_SETTING_PANEL_H

#include <QDialog>
#include <string>
#include "PRadCoordSystem.h"

class PRadDataHandler;
class PRadDetMatch;

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
    QDoubleSpinBox *coordBox[6]; // X, Y, Z, thetaX, thetaY, thetaZ
    QLineEdit *matchLine1;

private:
    bool hyCalGroup_data;
    bool gemGroup_data;
    std::vector<PRadCoordSystem::DetCoord> det_coords;
};

#endif
