#ifndef HISTVIEW_H
#define HISTVIEW_H

#include <openglwidget.h>
#include <yygl/glrenderpass.h>

class Hist;
class HistFacade;
class IHistPainter;
class IHistCharter;

class HistView : public OpenGLWidget {
    Q_OBJECT
public:
    explicit HistView(QWidget* parent = 0);
    typedef std::map<int, std::array<float, 2>> HistRangesMap;

signals:
    void selectedHistRangesChanged(HistRangesMap histRanges);

public:
    void setHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);

public:
    virtual int heightForWidth(int w) const override { return w; }
    virtual bool hasHeightForWidth() const override { return true; }

protected:
    virtual void paintGL() override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void leaveEvent(QEvent*) override;

private:
    std::shared_ptr<IHistCharter> _histCharter;
};

#endif // HISTVIEW_H
