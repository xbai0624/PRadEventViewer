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

#if QT_VERSION >= 0x500000
#include <QtWidgets>
#else
#include <QtGui>
#endif

ReconSettingPanel::ReconSettingPanel(QWidget *parent)
: QDialog(parent), handler(nullptr), coordSystem(nullptr), detMatch(nullptr)
{
    setWindowTitle(tr("Reconstruction Settings"));

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(createHyCalGroup(), 0, 0);
    grid->addWidget(createGEMGroup(), 0, 1);
    grid->addWidget(createCoordGroup(), 1, 0, 1, 2);
    grid->addWidget(createMatchGroup(), 2, 0, 1, 2);
    grid->addWidget(createStandardButtons(), 4, 0, 1, 2);

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
    QGroupBox *hyCalGroup = new QGroupBox(tr("Show HyCal Clusters"));
    hyCalGroup->setCheckable(true);
    hyCalGroup->setChecked(true);

    // methods combo box
    hyCalMethods = new QComboBox;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(hyCalMethods);

    hyCalGroup->setLayout(layout);

    return hyCalGroup;
}

QGroupBox *ReconSettingPanel::createGEMGroup()
{
    QGroupBox *gemGroup = new QGroupBox(tr("GEM Cluster Setting"));
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

// open Qt dialog, and save all the settings
int ReconSettingPanel::Execute()
{
    SaveSettings();
    return exec();
}

// save current settings
void ReconSettingPanel::SaveSettings()
{
    hyCalMethods_data = hyCalMethods->currentIndex();
}

// restore all the settings
void ReconSettingPanel::RestoreSettings()
{
    hyCalMethods->setCurrentIndex(hyCalMethods_data);
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
