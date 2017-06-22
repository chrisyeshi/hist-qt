#ifndef TIMEPLOTVIEW_H
#define TIMEPLOTVIEW_H

#include <openglwidget.h>

class DataPool;

class TimePlotView : public OpenGLWidget {
    Q_OBJECT
public:
    explicit TimePlotView(QWidget* parent = nullptr);

public:
    void setDataPool(DataPool* dataPool);

protected:
    virtual void paintGL() override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void leaveEvent(QEvent*) override;

signals:
    void timeStepChanged(int);

private:
    int localPosToStep(float x, float y) const;
    void setSelectedStep(int step);

private:
    static const QColor _hoveredColor;
    static const QColor _selectedColor;

private:
    DataPool* _data;
    int _hoveredStep, _selectedStep;
};

#endif // TIMEPLOTVIEW_H
