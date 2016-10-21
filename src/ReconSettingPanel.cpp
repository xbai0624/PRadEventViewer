//============================================================================//
// Derived from QDialog, provide a setting panel to Change Spectrum settings  //
//                                                                            //
// Chao Peng                                                                  //
// 02/17/2016                                                                 //
//============================================================================//

#include "ReconSettingPanel.h"
#include "PRadDataHandler.h"
#include "PRadHyCalCluster.h"
#include "PRadIslandCluster.h"
#include "PRadSquareCluster.h"
#include "PRadCoordSystem.h"
#include "PRadDetMatch.h"
#include "PRadGEMSystem.h"

#include <QFileDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>

ReconSettingPanel::ReconSettingPanel(QWidget *parent)
: QDialog(parent), handler(nullptr), coordSystem(nullptr), detMatch(nullptr)
{
    setWindowTitle(tr("Reconstruction Settings"));

    QVBoxLayout *grid = new QVBoxLayout;
    grid->addWidget(createHyCalGroup());
    grid->addWidget(createGEMGroup());
    grid->addWidget(createCoordGroup());
    grid->addWidget(createMatchGroup());
    grid->addWidget(createStandardButtons());

    setLayout(grid);
}

QDialogButtonBox *ReconSettingPanel::createStandardButtons()
{
    // Add standard buttons to layout
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // Connect standard buttons
    connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
                    this, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
                    this, SLOT(reject()));

    return buttonBox;
}

QGroupBox *ReconSettingPanel::createHyCalGroup()
{
    hyCalGroup = new QGroupBox(tr("Show HyCal Clusters"));
    hyCalGroup->setCheckable(true);
    hyCalGroup->setChecked(true);

    // methods combo box
    hyCalMethods = new QComboBox;
    // configuration file
    QPushButton *hyCalLoadConfig = new QPushButton(tr("Reload"));
    QPushButton *hyCalFindPath = new QPushButton(tr("Open Configuration File"));
    hyCalConfigPath = new QLineEdit;

    connect(hyCalMethods, SIGNAL(currentIndexChanged(int)), this, SLOT(updateHyCalPath()));
    connect(hyCalLoadConfig, SIGNAL(clicked()), this, SLOT(loadHyCalConfig()));
    connect(hyCalFindPath, SIGNAL(clicked()), this, SLOT(openHyCalConfig()));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(hyCalMethods, 0, 0);
    layout->addWidget(hyCalFindPath, 0, 1, 1, 2);
    layout->addWidget(hyCalConfigPath, 1, 0, 1, 2);
    layout->addWidget(hyCalLoadConfig, 1, 2);

    hyCalGroup->setLayout(layout);

    return hyCalGroup;
}

QGroupBox *ReconSettingPanel::createGEMGroup()
{
    gemGroup = new QGroupBox(tr("GEM Cluster Setting"));
    gemGroup->setCheckable(true);
    gemGroup->setChecked(true);

    gemLine1 = new QLineEdit;
    gemLine1->setValidator(new QIntValidator(0, 100, gemGroup));

    QFormLayout *layout = new QFormLayout;
    layout->addRow(tr("Place Holder"), gemLine1);

    gemGroup->setLayout(layout);

    return gemGroup;
}

QGroupBox *ReconSettingPanel::createCoordGroup()
{
    QGroupBox *typeGroup = new QGroupBox(tr("Coordinates Setting"));

    coordLine1 = new QLineEdit;

    QFormLayout *layout = new QFormLayout;
    layout->addRow(tr("Place Holder"), coordLine1);

    typeGroup->setLayout(layout);

    return typeGroup;
}

QGroupBox *ReconSettingPanel::createMatchGroup()
{
    QGroupBox *typeGroup = new QGroupBox(tr("Coincidence Setting"));

    matchLine1 = new QLineEdit;

    QFormLayout *layout = new QFormLayout;
    layout->addRow(tr("Place Holder"), matchLine1);

    typeGroup->setLayout(layout);

    return typeGroup;
}

void ReconSettingPanel::ConnectDataHandler(PRadDataHandler *h)
{
    handler = h;

    if(h == nullptr)
        return;

    // set the methods combo box according to the data handler settings
    hyCalMethods->clear();

    int index = -1; // means no value selected in combo box
    auto methods = handler->GetHyCalClusterMethodsList();
    auto current = handler->GetHyCalClusterMethodName();

    for(size_t i = 0; i < methods.size(); ++i)
    {
        if(methods.at(i) == current)
            index = i;

        hyCalMethods->addItem(QString::fromStdString(methods.at(i)));
    }

    hyCalMethods->setCurrentIndex(index);
}

void ReconSettingPanel::ConnectCoordSystem(PRadCoordSystem *c)
{
    coordSystem = c;
}

void ReconSettingPanel::ConnectMatchSystem(PRadDetMatch *m)
{
    detMatch = m;
}

void ReconSettingPanel::updateHyCalPath()
{
    std::string name = hyCalMethods->currentText().toStdString();
    PRadHyCalCluster *method = handler->GetHyCalClusterMethod(name);

    if(method)
        hyCalConfigPath->setText(QString::fromStdString(method->GetConfigPath()));
    else
        hyCalConfigPath->setText("");
}

// open the configuration file for selected method
void ReconSettingPanel::openHyCalConfig()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "HyCal Cluster Configuration File",
                                                "config/",
                                                "text config file (*.conf *.txt)");

    if(path.isEmpty())
        return;

    hyCalConfigPath->setText(path);
    loadHyCalConfig();
}

// load the configuration file for selected method
void ReconSettingPanel::loadHyCalConfig()
{
    handler->ConfigureHyCalClusterMethod(hyCalMethods->currentText().toStdString(),
                                         hyCalConfigPath->text().toStdString());
}

// open Qt dialog, and save all the settings
int ReconSettingPanel::Execute()
{
    SaveSettings();
    return exec();
}

// save current settings
void ReconSettingPanel::SaveSettings()
{
    hyCalGroup_data = hyCalGroup->isChecked();
    hyCalMethods_data = hyCalMethods->currentIndex();
    gemGroup_data = gemGroup->isChecked();
}

// restore all the settings
void ReconSettingPanel::RestoreSettings()
{
    hyCalGroup->setChecked(hyCalGroup_data);
    hyCalMethods->setCurrentIndex(hyCalMethods_data);
    gemGroup->setChecked(gemGroup_data);
}

// apply all the changes
void ReconSettingPanel::Apply()
{
    // set data handler
    if(handler != nullptr) {
        std::string method = hyCalMethods->currentText().toStdString();
        handler->SetHyCalClusterMethod(method);
    }
}

// overloaded, apply the settings if dialog is accepted
void ReconSettingPanel::accept()
{
    Apply();
    QDialog::accept();
}

// overloaded, restore the settings if dialog is rejected
void ReconSettingPanel::reject()
{
    RestoreSettings();
    QDialog::reject();
}

bool ReconSettingPanel::ShowHyCalCluster()
{
    return hyCalGroup->isChecked();
}

bool ReconSettingPanel::ShowGEMCluster()
{
    return gemGroup->isChecked();
}
