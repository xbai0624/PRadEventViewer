//============================================================================//
// Derived from QDialog, provide a setting panel to Change Cluster Display    //
//                                                                            //
// Chao Peng                                                                  //
// 10/24/2016                                                                 //
//============================================================================//

#include "ReconSettingPanel.h"
#include "PRadDataHandler.h"
#include "PRadHyCalCluster.h"
#include "PRadIslandCluster.h"
#include "PRadSquareCluster.h"
#include "PRadCoordSystem.h"
#include "PRadDetMatch.h"
#include "PRadGEMSystem.h"
#include "MarkSettingWidget.h"

#include <QFileDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QCheckBox>

#define COORD_ITEMS 6
#define MATCH_ITEMS 6

ReconSettingPanel::ReconSettingPanel(QWidget *parent)
: QDialog(parent), handler(nullptr), coordSystem(nullptr), detMatch(nullptr)
{
    setWindowTitle(tr("Reconstruction Settings"));

    // set the pointer container size
    coordBox.resize(COORD_ITEMS, nullptr);
    matchConfLabel.resize(MATCH_ITEMS, nullptr);
    matchConfBox.resize(MATCH_ITEMS, nullptr);
    markSettings.resize(PRadDetectors::Max_Dets, nullptr);

    QFormLayout *grid = new QFormLayout;
    grid->addRow(createMarkGroup());
    grid->addRow(createHyCalGroup(), createGEMGroup());
    grid->addRow(createCoordGroup());
    grid->addRow(createMatchGroup());
    grid->addRow(createStandardButtons());

    setLayout(grid);
}

QGroupBox *ReconSettingPanel::createMarkGroup()
{
    QGroupBox *markGroup = new QGroupBox(tr("Mark Settings"));
    QFormLayout *layout = new QFormLayout;

    QStringList mark_list;
    mark_list << "Circle" << "Cross" << "Triangle";
    QColor mark_color[] = {Qt::black, Qt::red, Qt::magenta};

    for(size_t i = 0; i < markSettings.size(); ++i)
    {
        markSettings[i] = new MarkSettingWidget(mark_list);
        markSettings[i]->SetColor(mark_color[i]);
        layout->addRow(tr(PRadDetectors::getName(i)), markSettings[i]);
    }

    markGroup->setLayout(layout);

    return markGroup;
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
    layout->addWidget(hyCalMethods, 1, 0);
    layout->addWidget(hyCalFindPath, 1, 1, 1, 2);
    layout->addWidget(hyCalConfigPath, 2, 0, 1, 2);
    layout->addWidget(hyCalLoadConfig, 2, 2);

    hyCalGroup->setLayout(layout);

    return hyCalGroup;
}

QGroupBox *ReconSettingPanel::createGEMGroup()
{
    gemGroup = new QGroupBox(tr("GEM Cluster Setting"));
    gemGroup->setCheckable(true);
    gemGroup->setChecked(true);

    QStringList mark_list;
    mark_list << "Circle" << "Cross" << "Triangle";

    gemMinHits = new QSpinBox;
    gemMinHits->setRange(1, 20);
    gemMaxHits = new QSpinBox;
    gemMaxHits->setRange(2, 100);
    gemSplitThres = new QDoubleSpinBox;
    gemSplitThres->setRange(1., 1000.);
    gemSplitThres->setSingleStep(0.1);

    QFormLayout *layout = new QFormLayout;
    layout->addRow(tr("Min. Cluster Hits"), gemMinHits);
    layout->addRow(tr("Max, Cluster Hits"), gemMaxHits);
    layout->addRow(tr("Split Threshold"), gemSplitThres);

    gemGroup->setLayout(layout);

    return gemGroup;
}

QGroupBox *ReconSettingPanel::createCoordGroup()
{
    QGroupBox *typeGroup = new QGroupBox(tr("Coordinates Setting"));

    QLabel *ax = new QLabel("Axis");
    QLabel *xl = new QLabel("  X");
    QLabel *yl = new QLabel("  Y");
    QLabel *zl = new QLabel("  Z");
    QLabel *off = new QLabel("Origin (mm)");
    QLabel *tilt = new QLabel("Angle (mrad)");

    coordType = new QComboBox;
    coordRun = new QComboBox;
    QPushButton *restoreCoord = new QPushButton("Restore");
    QPushButton *saveCoord = new QPushButton("Save");
    QPushButton *loadCoordFile = new QPushButton("Open Coord File");
    QPushButton *saveCoordFile = new QPushButton("Save Coord File");

    int decimals[]     = {    4,     4,     1,     4,     4,     4};
    double step[]      = { 1e-4,  1e-4,  1e-1,  1e-4,  1e-4,  1e-4};
    double min_range[] = {  -20,   -20,     0,   -20,   -20,   -20};
    double max_range[] = {   20,    20, 10000,    20,    20,    20};

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(coordRun, 0, 0);
    layout->addWidget(loadCoordFile, 0, 2);
    layout->addWidget(saveCoordFile, 0, 3);
    layout->addWidget(coordType, 1, 0);
    layout->addWidget(saveCoord, 1, 2);
    layout->addWidget(restoreCoord, 1, 3);
    layout->addWidget(ax, 2, 0);
    layout->addWidget(xl, 2, 1);
    layout->addWidget(yl, 2, 2);
    layout->addWidget(zl, 2, 3);
    layout->addWidget(off,3, 0);
    layout->addWidget(tilt, 4, 0);

    for(size_t i = 0; i < coordBox.size(); ++i)
    {
        // setting coord spin box
        coordBox[i] = new QDoubleSpinBox;
        coordBox[i]->setDecimals(decimals[i]);
        coordBox[i]->setRange(min_range[i], max_range[i]);
        coordBox[i]->setSingleStep(step[i]);

        // add to layout
        int row = 3 + i/3;
        int col = 1 + i%3;
        layout->addWidget(coordBox[i], row, col);
    }

    typeGroup->setLayout(layout);

    connect(coordRun, SIGNAL(currentIndexChanged(int)), this, SLOT(selectCoordData(int)));
    connect(coordType, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCoordType(int)));
    connect(restoreCoord, SIGNAL(clicked()), this, SLOT(restoreCoordData()));
    connect(saveCoord, SIGNAL(clicked()), this, SLOT(saveCoordData()));
    connect(loadCoordFile, SIGNAL(clicked()), this, SLOT(openCoordFile()));
    connect(saveCoordFile, SIGNAL(clicked()), this, SLOT(saveCoordFile()));

    return typeGroup;
}

QGroupBox *ReconSettingPanel::createMatchGroup()
{
    QGroupBox *matchGroup = new QGroupBox(tr("Coincidence Setting"));

    QGridLayout *layout = new QGridLayout;

    matchHyCal = new QCheckBox("Show Matched HyCal Clusters Only");
    matchGEM = new QCheckBox("Show Matched GEM Clusters Only");

    layout->addWidget(matchHyCal, 0, 0, 1, 2);
    layout->addWidget(matchGEM, 0, 2, 1, 2);

    QStringList matchDescript;
    matchDescript << "Lead Glass Resolution" << "Transition Resolution"
                  << "Crystal Resolution" << "Match Factor"
                  << "GEM Resolution" << "GEM Overlap Factor";

    for(size_t i = 0; i < matchConfBox.size(); ++i)
    {
        // label
        matchConfLabel[i] = new QLabel(matchDescript[i]);
        // conf spin box
        matchConfBox[i] = new QDoubleSpinBox;
        matchConfBox[i]->setRange(0., 50.);
        matchConfBox[i]->setSingleStep(0.01);
        // add to layout
        layout->addWidget(matchConfLabel[i], 2+i/2, (i%2)*2);
        layout->addWidget(matchConfBox[i], 2+i/2, (i%2)*2 + 1);
    }

    matchGroup->setLayout(layout);

    return matchGroup;
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

void ReconSettingPanel::ConnectDataHandler(PRadDataHandler *h)
{
    handler = h;

    // set the methods combo box according to the data handler settings
    hyCalMethods->clear();

    if(h == nullptr)
        return;

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

    // set the config values from GEM clustering method
    PRadGEMCluster *gem_method = handler->GetSRS()->GetClusterMethod();

    gemMinHits->setValue(gem_method->GetConfigValue("MIN_CLUSTER_HITS").Int());
    gemMaxHits->setValue(gem_method->GetConfigValue("MAX_CLUSTER_HITS").Int());
    gemSplitThres->setValue(gem_method->GetConfigValue("SPLIT_CLUSTER_DIFF").Double());
}

void ReconSettingPanel::ConnectCoordSystem(PRadCoordSystem *c)
{
    coordSystem = c;

    coordRun->clear();
    coordType->clear();

    if(c == nullptr)
        return;

    for(auto &it : c->GetCoordsData())
    {
        coordRun->addItem(QString::number(it.first));
    }

    det_coords = c->GetCurrentCoords();

    for(auto &coord : det_coords)
    {
        coordType->addItem(PRadDetectors::getName(coord.det_enum));
    }

    coordType->setCurrentIndex(0);
}

void ReconSettingPanel::ConnectMatchSystem(PRadDetMatch *m)
{
    detMatch = m;

    for(size_t i = 0; i < matchConfBox.size(); ++i)
    {
        float value = 0.;
        if(detMatch != nullptr)
            value = detMatch->GetConfigValue(matchConfLabel[i]->text().toStdString()).Float();

        matchConfBox[i]->setValue(value);
    }
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

void ReconSettingPanel::changeCoordType(int t)
{
    if(coordSystem == nullptr)
        return;

    if((size_t)t > coordBox.size()) { // -1 is the default nohting to select value
        for(auto &box : coordBox)
            box->setValue(0);
        return;
    }

    auto &coord = det_coords[t];

    for(size_t i = 0; i < coordBox.size(); ++i)
    {
        coordBox[i]->setValue(coord.get_dim_coord(i));
    }

}

void ReconSettingPanel::selectCoordData(int r)
{
    if(r < 0 || coordSystem == nullptr)
        return;

    coordSystem->ChooseCoord(r);
    det_coords = coordSystem->GetCurrentCoords();
    changeCoordType(coordType->currentIndex());
}

void ReconSettingPanel::saveCoordData()
{
    size_t idx = (size_t)coordType->currentIndex();
    if(idx >= det_coords.size())
        return;

    auto &coord = det_coords[idx];

    for(size_t i = 0; i < coordBox.size(); ++i)
    {
        coord.set_dim_coord(i, coordBox[i]->value());
    }
}

void ReconSettingPanel::saveCoordFile()
{
    QString path = QFileDialog::getSaveFileName(this,
                                                "Coordinates File",
                                                "config/",
                                                "text data file (*.dat *.txt)");
    if(path.isEmpty())
        return;

    saveCoordData();
    coordSystem->SetCurrentCoord(det_coords);
    coordSystem->SaveCoordData(path.toStdString());
}

void ReconSettingPanel::openCoordFile()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Coordinates File",
                                                "config/",
                                                "text data file (*.dat *.txt)");
    if(path.isEmpty())
        return;

    coordSystem->LoadCoordData(path.toStdString());

    // re-connect to apply changes
    ConnectCoordSystem(coordSystem);
}

void ReconSettingPanel::restoreCoordData()
{
    if(coordSystem == nullptr)
        return;

    det_coords = coordSystem->GetCurrentCoords();
    changeCoordType(coordType->currentIndex());
}

// reconnects all the objects to read their settings
void ReconSettingPanel::SyncSettings()
{
    ConnectDataHandler(handler);
    ConnectCoordSystem(coordSystem);
    ConnectMatchSystem(detMatch);
}

// save the settings that cannot by sync
void ReconSettingPanel::SaveSettings()
{
    hyCalGroup_data = hyCalGroup->isChecked();
    gemGroup_data = gemGroup->isChecked();
    matchHyCal_data = matchHyCal->isChecked();
    matchGEM_data = matchGEM->isChecked();
    for(auto &mark : markSettings)
        mark->SaveSettings();
}

// restore the saved settings
void ReconSettingPanel::RestoreSettings()
{
    hyCalGroup->setChecked(hyCalGroup_data);
    gemGroup->setChecked(gemGroup_data);
    matchHyCal->setChecked(matchHyCal_data);
    matchGEM->setChecked(matchGEM_data);
    for(auto &mark : markSettings)
        mark->RestoreSettings();
}

// apply all the changes
void ReconSettingPanel::ApplyChanges()
{
    // set data handler
    if(handler != nullptr) {

        // change HyCal clustering method
        std::string method = hyCalMethods->currentText().toStdString();
        handler->SetHyCalClusterMethod(method);

        // set corresponding gem cluster configuration values
        PRadGEMCluster *gem_method = handler->GetSRS()->GetClusterMethod();
        gem_method->SetConfigValue("MIN_CLUSTER_HITS", gemMinHits->value());
        gem_method->SetConfigValue("MAX_CLUSTER_HITS", gemMaxHits->value());
        gem_method->SetConfigValue("SPLIT_CLUSTER_DIFF", gemSplitThres->value());
        // reaload the configuration
        gem_method->Configure();
    }

    // set coordinate system
    if(coordSystem != nullptr) {
        saveCoordData();
        coordSystem->SetCurrentCoord(det_coords);
    }

    if(detMatch != nullptr) {
        for(size_t i = 0; i < matchConfBox.size(); ++i)
        {
            detMatch->SetConfigValue(matchConfLabel[i]->text().toStdString(), matchConfBox[i]->value());
        }
    }
}

bool ReconSettingPanel::ShowHyCalCluster()
{
    return hyCalGroup->isChecked();
}

bool ReconSettingPanel::ShowMatchedHyCal()
{
    return matchHyCal->isChecked();
}

bool ReconSettingPanel::ShowGEMCluster()
{
    return gemGroup->isChecked();
}

bool ReconSettingPanel::ShowMatchedGEM()
{
    return matchGEM->isChecked();
}

int ReconSettingPanel::GetMarkIndex(int det)
{
    if(det < 0 || det >= PRadDetectors::Max_Dets)
        return -1;

    return markSettings[det]->GetCurrentMarkIndex();
}

QString ReconSettingPanel::GetMarkName(int det)
{
    if(det < 0 || det >= PRadDetectors::Max_Dets)
        return "";

    return markSettings[det]->GetCurrentMarkName();
}

QColor ReconSettingPanel::GetMarkColor(int det)
{
    if(det < 0 || det >= PRadDetectors::Max_Dets)
        return Qt::black;

    return markSettings[det]->GetColor();
}
