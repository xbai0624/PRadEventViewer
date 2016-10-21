#ifndef RECON_SETTING_PANEL_H
#define RECON_SETTING_PANEL_H

#include <QDialog>
#include <string>

class PRadDataHandler;
class PRadCoordSystem;
class PRadDetMatch;

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
    QLineEdit *gemLine1;
    QLineEdit *coordLine1;
    QLineEdit *matchLine1;

private:
    //saved settings
    bool hyCalGroup_data;
    bool gemGroup_data;
    int hyCalMethods_data;
};

#endif
