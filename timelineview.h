#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <openglwidget.h>
#include <data/DataPool.h>

class QSlider;
class QScrollBar;
class TimePlotView;

class TimelineView : public OpenGLWidget
{
    Q_OBJECT
public:
    explicit TimelineView(QWidget *parent = 0);

public:
    void setTimeStep(int timeStep);
    void setStepCount(int stepCount) { _nSteps = stepCount; }
    void setHistConfig(HistConfig histConfig) { _histConfig = histConfig; }
    void setStats(DataPool::Stats dataStats) { _dataStats = dataStats; }
    void setDisplayDims(std::vector<int> displayDims) {
        _displayDims = displayDims;
    }

signals:
    void timeStepChanged(int);

protected:
    virtual void paintGL() override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void leaveEvent(QEvent*) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

private:
    int localPosToStep(QPointF pos) const;
    void setSelectedStep(int step);

private:
    static const QColor _hoveredColor;
    static const QColor _selectedColor;

private:
//    TimePlotView* _timePlotView;
    int _nSteps, _hoveredStep, _currStep;
    HistConfig _histConfig;
    DataPool::Stats _dataStats;
    std::string _histVolumeName;
    std::vector<int> _displayDims;
    QRectF _plotRect;
};

#endif // TIMELINEVIEW_H
