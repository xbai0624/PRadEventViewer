#ifndef RECON_SETTING_PANEL_H
#define RECON_SETTING_PANEL_H

#include <string>
#include <vector>
#include <QDialog>
#include "PRadCoordSystem.h"
#include "PRadDetectors.h"

class PRadDataHandler;
class PRadDetMatch;
class MarkSettingWidget;

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

    // connect objects
    void ConnectDataHandler(PRadDataHandler *h);
    void ConnectCoordSystem(PRadCoordSystem *c);
    void ConnectMatchSystem(PRadDetMatch *m);

    // change related
    void SyncSettings();
    void SaveSettings();
    void RestoreSettings();
    void ApplyChanges();

    // get data
    bool ShowHyCalCluster();
    bool ShowGEMCluster();
    bool ShowMatchedHyCal();
    bool ShowMatchedGEM();
    int GetMarkIndex(int det);
    QString GetMarkName(int det);
    QColor GetMarkColor(int det);

private:
    QGroupBox *createMarkGroup();
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
    std::vector<QDoubleSpinBox*> coordBox; // X, Y, Z, thetaX, thetaY, thetaZ

    QCheckBox *matchHyCal;
    QCheckBox *matchGEM;
    std::vector<QLabel*> matchConfLabel;
    std::vector<QDoubleSpinBox*> matchConfBox; // resolution and matching factors

    std::vector<MarkSettingWidget*> markSettings;

private:
    bool hyCalGroup_data;
    bool gemGroup_data;
    bool matchHyCal_data;
    bool matchGEM_data;
    std::vector<PRadCoordSystem::DetCoord> det_coords;
};

#endif
