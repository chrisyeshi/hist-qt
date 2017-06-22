#ifndef HISTSLICEVIEW_H
#define HISTSLICEVIEW_H

#include <openglwidget.h>
#include <array>
#include <memory>

class HistFacadeRect;
class IHistPainter;

/**
 * @brief The HistSliceView class
 */
class HistSliceView : public OpenGLWidget {
    Q_OBJECT
public:
    explicit HistSliceView(QWidget* parent);

signals:
    void histClicked(std::array<int, 2> rectIds, std::vector<int> dims);
    void histHovered(
            std::array<int, 2> rectIds, std::vector<int> dims, bool hovered);

public:
    void setHistRect(std::shared_ptr<HistFacadeRect> histRect);
    void setHistDimensions(std::vector<int> histDims);
    void setSpacingColor(QColor color);
    void setClickedHist(std::array<int,2> rectId, bool clicked = true);
    void setHoveredHist(std::array<int,2> rectId, bool hovered = true);
    void updateHistPainters();

protected:
    virtual void paintGL() override;
    virtual void resizeGL(int, int) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

private:
    void updateHistRects(int w, int h);
    float perHistWidthPixels(int w);
    float perHistHeightPixels(int h);
    int scaledWidth() { return width() * devicePixelRatio(); }
    int scaledHeight() { return height() * devicePixelRatio(); }
    QColor fullColor() const;
    QColor halfColor() const;

private:
    std::shared_ptr<HistFacadeRect> _histRect;
    std::vector<std::shared_ptr<IHistPainter>> _histPainters;
    std::vector<int> _histDims;
    QColor _spacingColor;
    QPointF _mousePress;
    bool _histClicked = false;
    std::array<int,2> _clickedHistRectId;
    bool _histHovered = false;
    std::array<int,2> _hoveredHistRectId;

private:
    static constexpr float _border = 4.f;
    static constexpr float _spacing = 1.f;
};

#endif // HISTSLICEVIEW_H
