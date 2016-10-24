#ifndef MARK_SETTING_WIDGET_H
#define MARK_SETTING_WIDGET_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QPushButton;

class MarkSettingWidget : public QWidget
{
    Q_OBJECT

public:
    MarkSettingWidget(QWidget *parent = 0);
    MarkSettingWidget(const QStringList &m, QWidget *parent = 0);
    ~MarkSettingWidget() {};

    void SaveSettings();
    void RestoreSettings();

    void SetMarkNames(const QStringList &n);
    void AddMarkName(const QString &n);
    void ClearMarkNames();
    void SetCurrentMarkIndex(int i);
    void SetColor(const QColor &c);
    void SetColor(const QString &c_text);

    QString GetCurrentMarkName();
    int GetCurrentMarkIndex();
    QColor GetColor();

private slots:
    void colorLabelChange(const QString &);
    void colorPicker();

private:
    void initialize();

    QComboBox *markCombo;
    QLineEdit *colorEdit;
    QPushButton *colorButton;
    QColor savedColor;
    int savedIndex;
};

#endif
