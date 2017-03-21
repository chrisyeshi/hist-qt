#ifndef HISTSLICEORIENVIEW_H
#define HISTSLICEORIENVIEW_H

#include <QOpenGLWidget>
#include <data/Extent.h>
#include <yygl/glrenderpass.h>
#include <array>

class Camera;

/**
 * @brief The HistSliceOrienView class
 */
class HistSliceOrienView : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit HistSliceOrienView(QWidget* parent);

public:
    void setHistDims(int w, int h, int d);
    void setHistDims(const Extent &dims);
    void highlightXYSlice(int index);
    void highlightXZSlice(int index);
    void highlightYZSlice(int index);
    void setHoveredHist(std::array<int, 3> histIds, bool hovered);

protected:
    virtual void initializeGL() override;
    virtual void paintGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateMVP();
    QMatrix4x4 yzSliceMatrix(int index) const;
    QMatrix4x4 xzSliceMatrix(int index) const;
    QMatrix4x4 xySliceMatrix(int index) const;
    QMatrix4x4 hoveredHistMatrix(std::array<int, 3> histIds);

private:
    static const int NUM_SLICES = 3;
    static const int YZ = 0;
    static const int XZ = 1;
    static const int XY = 2;

private:
    Extent _dimHists;
    yy::gl::render_pass _boundingBoxRenderPass;
    std::array<yy::gl::render_pass, NUM_SLICES> _highlightRenderPasses;
    std::shared_ptr<yy::gl::program> _highlightProgram;
    std::array<int, NUM_SLICES> _sliceIndices;
    yy::gl::vector<glm::vec3> _cube;
    std::shared_ptr<Camera> _camera;
    QPointF _mousePrev;
    yy::gl::render_pass _hoveredRenderPass;
    bool _histHovered;
    std::array<int, 3> _hoveredHistIds;
};

#endif // HISTSLICEORIENVIEW_H
