#ifndef HISTSLICEVIEW_H
#define HISTSLICEVIEW_H

#include <openglwidget.h>
#include <mask.h>
#include <array>

class HistFacadeRect;
class HistPainter;

/**
 * @brief The HistSliceView class
 */
class HistSliceView : public OpenGLWidget {
    Q_OBJECT
public:
    explicit HistSliceView(QWidget* parent);

signals:
    void histClicked(std::array<int, 2> rectIds, std::vector<int> dims);

public:
    void setHistRect(std::shared_ptr<HistFacadeRect> histRect);
    void setSelectedHistMask(BoolMask2D selectedHistMask);
    void setHistDimensions(std::vector<int> histDims);
    void update();

protected:
    virtual void paintGL() override;
    virtual void resizeGL(int, int) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateHistPainters();

private:
    std::shared_ptr<HistFacadeRect> _histRect;
    BoolMask2D _selectedHistMask;
    std::vector<std::shared_ptr<HistPainter>> _histPainters;
    std::vector<int> _histDims;
};

#endif // HISTSLICEVIEW_H
