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
    enum DrawMode {BarChart, LineChart};

public:
    void setTimeStep(int timeStep);
    void setTimeSteps(const TimeSteps& timeSteps);
    void setHistConfig(HistConfig histConfig) { _histConfig = histConfig; }
    void setStats(DataPool::Stats dataStats);
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
    void drawTimelineAsBarChart();
    void drawTimelineAsLineChart();

private:
    static const QColor _hoveredColor;
    static const QColor _selectedColor;

private:
    int _hoveredStep, _currStep;
    TimeSteps _timeSteps;
    HistConfig _histConfig;
    DataPool::Stats _dataStats;
    std::string _histVolumeName;
    std::vector<int> _displayDims;
    QRectF _plotRect;
    DrawMode _drawMode = LineChart;
};

#endif // TIMELINEVIEW_H
