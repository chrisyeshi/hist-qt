#ifndef HISTVIEW_H
#define HISTVIEW_H

#include <openglwidget.h>
#include <yygl/glrenderpass.h>

class Hist;
class HistFacade;
class IHistPainter;
class IHistCharter;
class Painter;

class HistView : public OpenGLWidget {
    Q_OBJECT
public:
    explicit HistView(QWidget* parent = 0);

public:
    void setHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);

protected:
    virtual void paintGL() override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void leaveEvent(QEvent*) override;

private:
    std::shared_ptr<IHistCharter> _histCharter;
};

#endif // HISTVIEW_H
