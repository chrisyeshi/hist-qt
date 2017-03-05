#ifndef HISTVIEW_H
#define HISTVIEW_H

#include <openglwidget.h>
#include <yygl/glrenderpass.h>

class Hist;
class HistPainter;

/// TODO: consider one HistView to render all different histograms, use
/// different HistPainter for rendering. However, HistView should draw the axes
/// while HistPainter only draws the histogram.
/// TODO: Consider setting HistFacade instead of Hist, so that HistView can also
/// modify the selected attribute of the Histograms.
class HistView : public OpenGLWidget
{
public:
    explicit HistView(QWidget* parent = 0);
};

class Hist2DView : public HistView {
public:
    explicit Hist2DView(QWidget* parent = 0);

public:
    void setHist(std::shared_ptr<const Hist> hist);

protected:
    virtual void paintGL() override;
    virtual void resizeGL(int w, int h) override;

private:
    std::shared_ptr<const Hist> _hist;
    std::shared_ptr<HistPainter> _painter;
};

#endif // HISTVIEW_H
