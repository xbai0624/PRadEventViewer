#ifndef PRAD_HYCAL_SCENE_H
#define PRAD_HYCAL_SCENE_H

#include <QGraphicsScene>
#include <vector>
#include <unordered_map>

class PRadEventViewer;
class HyCalModule;

class HyCalScene : public QGraphicsScene
{
    Q_OBJECT

public:
    struct TextBox
    {
        QString name;
        QString text;
        QColor textColor;
        QRectF bound;
        QColor bgColor;

        TextBox() {};
        TextBox(const QString &n, const QString &t, const QColor &tc, const QRectF &b, const QColor &c)
        : name(n), text(t), textColor(tc), bound(b), bgColor(c) {};
        TextBox(const QString &n, const QColor &tc, const QRectF &b, const QColor &c)
        : name(n), text(""), textColor(tc), bound(b), bgColor(c) {};
    };

    struct HitsMark
    {
        QString name;
        QString text;
        QColor textColor;
        QRectF textBox;
        QPointF hitPos;
        QColor hitColor;
        float size;

        HitsMark() {};
        HitsMark(const QString &n, const QString &t, const QPointF &p, const QColor &c, const float &s)
        : name(n), text(t), textColor(Qt::black), textBox(QRectF(p.x()-50., p.y()-20., 100., 40.)),
          hitPos(p), hitColor(c), size(s)
        {};
    };

    HyCalScene(PRadEventViewer *p, QObject *parent = 0)
    : QGraphicsScene(parent), console(p),
      pModule(nullptr), sModule(nullptr), rModule(nullptr), showScalers(false)
    {};

    HyCalScene(PRadEventViewer *p, const QRectF & sceneRect, QObject *parent = 0)
    : QGraphicsScene(sceneRect, parent), console(p),
      pModule(nullptr), sModule(nullptr), rModule(nullptr), showScalers(false)
    {};

    HyCalScene(PRadEventViewer*p, qreal x, qreal y, qreal width, qreal height, QObject *parent = 0)
    : QGraphicsScene(x, y, width, height, parent), console(p),
      pModule(nullptr), sModule(nullptr), rModule(nullptr), showScalers(false)
    {};

    void AddTDCBox(const QString &name, const QColor &textColor, const QRectF &textBox, const QColor &bgColor);
    void AddScalerBox(const QString &name, const QColor &textColor, const QRectF &textBox, const QColor &bgColor);
    void AddHitsMark(const QString &name, const QPointF &position, const QColor &markColor, const float &markSize, const QString &text = "");
    void ClearHitsMarks();
    void UpdateScalerBox(const QString &text, const int &group = 0);
    void UpdateScalerBox(const QStringList &texts);
    void ShowScalers(const bool &s = true) {showScalers = s;};
    void addModule(HyCalModule *module);
    void addItem(QGraphicsItem *item);
    QVector<HyCalModule *> GetModuleList() {return moduleList;};

protected:
    void drawForeground(QPainter *painter, const QRectF &rect);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    void drawScalerBoxes(QPainter *painter);
    void drawTDCBoxes(QPainter *painter);
    void drawHitsMarks(QPainter *painter);

private:
    PRadEventViewer *console;
    HyCalModule *pModule, *sModule, *rModule;
    bool showScalers;
    QList<TextBox> tdcBoxList;
    QVector<TextBox> scalarBoxList;
    QVector<HitsMark> hitsMarkList;
    QVector<HyCalModule *> moduleList;
};

#endif
