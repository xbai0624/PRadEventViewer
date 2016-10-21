#ifndef RECON_SETTING_PANEL_H
#define RECON_SETTING_PANEL_H

#include <QDialog>
#include <string>

class PRadDataHandler;
class PRadCoordSystem;
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
    int Execute();
    void ConnectDataHandler(PRadDataHandler *h);
    void ConnectCoordSystem(PRadCoordSystem *c);
    void ConnectMatchSystem(PRadDetMatch *m);
    void SyncSettings();
    void SaveSettings();
    void RestoreSettings();
    void Apply();
    bool ShowHyCalCluster();
    bool ShowGEMCluster();

public slots:
    void accept();
    void reject();

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

    QLineEdit *coordLine1;
    QLineEdit *matchLine1;

private:
    bool hyCalGroup_data;
    bool gemGroup_data;
};

#endif
