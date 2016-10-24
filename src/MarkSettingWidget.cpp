//============================================================================//
// A simple widget to set up the mark shape and color                         //
//                                                                            //
// Chao Peng                                                                  //
// 10/24/2016                                                                 //
//============================================================================//

#include "MarkSettingWidget.h"
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

MarkSettingWidget::MarkSettingWidget(QWidget *parent)
: QWidget(parent), savedColor(Qt::black), savedIndex(-1)
{
    initialize();
}

MarkSettingWidget::MarkSettingWidget(const QStringList &m, QWidget *parent)
: QWidget(parent), savedColor(Qt::black), savedIndex(-1)
{
    initialize();
    SetMarkNames(m);
}

void MarkSettingWidget::AddMarkName(const QString &n)
{
    markCombo->addItem(n);
}

void MarkSettingWidget::initialize()
{
    // methods combo box
    QHBoxLayout *layout = new QHBoxLayout;
    QLabel *markLabel = new QLabel(tr("Mark Shape"));
    markCombo = new QComboBox;

    QLabel *colorLabel = new QLabel(tr("Mark Color #"));
    colorEdit = new QLineEdit;
    colorEdit->setInputMask("HHHHHH;_");

    colorButton = new QPushButton;
    colorButton->setMaximumWidth(30);
    colorButton->setMaximumHeight(25);

    layout->addWidget(markLabel);
    layout->addWidget(markCombo);
    layout->addWidget(colorLabel);
    layout->addWidget(colorEdit);
    layout->addWidget(colorButton);
    layout->setContentsMargins(0, 0, 0, 0);

    connect(colorEdit, SIGNAL(textChanged(const QString&)), this, SLOT(colorLabelChange(const QString&)));
    connect(colorButton, SIGNAL(clicked()), this, SLOT(colorPicker()));

    setLayout(layout);
}

void MarkSettingWidget::SetMarkNames(const QStringList &n)
{
    markCombo->clear();
    for(auto &name : n)
    {
        markCombo->addItem(name);
    }
}

void MarkSettingWidget::ClearMarkNames()
{
    markCombo->clear();
}

int MarkSettingWidget::GetCurrentMarkIndex()
{
    return markCombo->currentIndex();
}

QString MarkSettingWidget::GetCurrentMarkName()
{
    return markCombo->currentText();
}

QColor MarkSettingWidget::GetColor()
{
    if(colorEdit->text().size() != 6)
        return Qt::black;

    return QColor("#" + colorEdit->text());
}

void MarkSettingWidget::SetCurrentMarkIndex(int i)
{
    markCombo->setCurrentIndex(i);
}

void MarkSettingWidget::SetColor(const QColor &q)
{
    colorEdit->setText(q.name().mid(1));
}

void MarkSettingWidget::SetColor(const QString &c_text)
{
    if(c_text.at(0) == '#')
        colorEdit->setText(c_text.mid(1));
    else
        colorEdit->setText(c_text);
}

void MarkSettingWidget::SaveSettings()
{
    savedIndex = GetCurrentMarkIndex();
    savedColor = GetColor();
}

void MarkSettingWidget::RestoreSettings()
{
    SetColor(savedColor);
    SetCurrentMarkIndex(savedIndex);
}

void MarkSettingWidget::colorPicker()
{
    QColor color = QColorDialog::getColor();

    if(!color.isValid())
        return;

    // mid(1) to get rid of the first '#
    colorEdit->setText(color.name().mid(1));
}

void MarkSettingWidget::colorLabelChange(const QString &color_str)
{
    if(color_str.size() != 6)
        return;

    colorButton->setStyleSheet("background: #" + color_str + ";");
    colorButton->update();
}
