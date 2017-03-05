#include "histsliceorienview.h"
#include <camera.h>
#include <QMouseEvent>

HistSliceOrienView::HistSliceOrienView(QWidget *parent)
  : QOpenGLWidget(parent)
  , _dimHists(2, 2, 2)
  , _sliceIndices({0, 0, 0})
  , _cube({
        { -1.f, -1.f, -1.f },
        {  1.f, -1.f, -1.f },
        { -1.f,  1.f, -1.f },
        {  1.f,  1.f, -1.f },
        { -1.f, -1.f,  1.f },
        {  1.f, -1.f,  1.f },
        { -1.f,  1.f,  1.f },
        {  1.f,  1.f,  1.f }
    })
  , _camera(new Camera())
{

}

void HistSliceOrienView::setHistDims(int w, int h, int d)
{
    setHistDims(Extent(w, h, d));
}

void HistSliceOrienView::setHistDims(const Extent& dims)
{
    _dimHists = dims;
    updateMVP();
    update();
}

void HistSliceOrienView::highlightXYSlice(int index)
{
    _sliceIndices[XY] = index;
    updateMVP();
    update();
}

void HistSliceOrienView::highlightXZSlice(int index)
{
    _sliceIndices[XZ] = index;
    updateMVP();
    update();
}

void HistSliceOrienView::highlightYZSlice(int index)
{
    _sliceIndices[YZ] = index;
    updateMVP();
    update();
}

void HistSliceOrienView::highlightHist(int flatId)
{
    update();
}

void HistSliceOrienView::initializeGL()
{
    glClearColor(1.f, 1.f, 1.f, 1.f);
    // bounding box render pass
    _boundingBoxRenderPass.setProgram(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                uniform mat4 mvp;
                in vec4 v_position;
                void main() {
                    gl_Position = mvp * v_position;
                }
            )GLSL",
            yy::gl::shader::FRAGMENT_SHADER,
            R"GLSL(
                #version 330
                out vec4 f_color;
                void main() {
                    f_color = vec4(0.4, 0.4, 0.4, 1.0);
                }
            )GLSL");
    _boundingBoxRenderPass.setVBO<glm::vec3>("v_position", _cube);
    _boundingBoxRenderPass.setIBO<GLuint>({
        0, 1, 0, 2, 2, 3, 1, 3, 4, 5, 4, 6, 6, 7, 5, 7, 0, 4, 1, 5, 2, 6, 3, 7
    });
    _boundingBoxRenderPass.setDrawMode(yy::gl::render_pass::LINES);
    _boundingBoxRenderPass.setFirstVertexIndex(0);
    _boundingBoxRenderPass.setVertexCount(24);
    // highlight region shader program
    _highlightProgram = std::make_shared<yy::gl::program>(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                uniform mat4 mvp;
                in vec4 v_position;
                void main() {
                    gl_Position = mvp * v_position;
                }
            )GLSL",
            yy::gl::shader::FRAGMENT_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 color;
                out vec4 f_color;
                void main() {
                    f_color = color;
                }
            )GLSL");
    // highlight render passes
    for (int dir = 0; dir < NUM_SLICES; ++dir) {
        yy::gl::render_pass& pass = _highlightRenderPasses[dir];
        pass.setProgram(_highlightProgram);
        pass.setVBO("v_position", _cube);
        /// TODO: also share the IBO.
        pass.setIBO<GLuint>({
            0, 1, 2, 1, 3, 2, /* xy min */
            0, 2, 6, 0, 6, 4, /* yz min */
            4, 7, 5, 4, 6, 7, /* xy max */
            1, 5, 3, 3, 5, 7, /* yz max */
            2, 3, 6, 6, 3, 7, /* xz max */
            0, 4, 1, 1, 4, 5  /* xz min */
        });
        pass.setDrawMode(yy::gl::render_pass::TRIANGLES);
        pass.setFirstVertexIndex(0);
        pass.setVertexCount(36);
    }
    _highlightRenderPasses[YZ].setUniform(
            "color", glm::vec4(1.f, 0.f, 0.f, 0.2f));
    _highlightRenderPasses[XZ].setUniform(
            "color", glm::vec4(0.f, 1.f, 0.f, 0.2f));
    _highlightRenderPasses[XY].setUniform(
            "color", glm::vec4(0.f, 0.f, 1.f, 0.2f));
    // camera
    _camera->reset({ -1.f, -1.f, -1.f }, { 1.f, 1.f, 1.f });
    updateMVP();
}

void HistSliceOrienView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _boundingBoxRenderPass.drawElements();
    for (auto& pass : _highlightRenderPasses)
        pass.drawElements();
    glDisable(GL_BLEND);
}

void HistSliceOrienView::resizeGL(int w, int h)
{
    _camera->setAspectRatio(float(w) / float(h));
    updateMVP();
    update();
}

void HistSliceOrienView::mousePressEvent(QMouseEvent *event)
{
    mousePrev = event->localPos();
}

void HistSliceOrienView::mouseReleaseEvent(QMouseEvent *event)
{

}

void HistSliceOrienView::mouseMoveEvent(QMouseEvent *event)
{
    auto mouseCurr = event->localPos();
    auto mouseDir =
            QVector2D(
                (mouseCurr - mousePrev) / (height() * devicePixelRatio()));
    mouseDir.setY(-mouseDir.y());
    if (event->buttons() & Qt::LeftButton)
        _camera->orbit(mouseDir);
    // reset near and far plane
    _camera->resetNearFar({ -1.f, -1.f, -1.f }, { 1.f, 1.f, 1.f });
    updateMVP();
    mousePrev = mouseCurr;
    update();
}

void HistSliceOrienView::updateMVP()
{
    auto vp = _camera->matProj() * _camera->matView();
    _boundingBoxRenderPass.setUniform("mvp", vp);
    _highlightRenderPasses[YZ].setUniform(
            "mvp", vp * yzSliceMatrix(_sliceIndices[YZ]));
    _highlightRenderPasses[XZ].setUniform(
            "mvp", vp * xzSliceMatrix(_sliceIndices[XZ]));
    _highlightRenderPasses[XY].setUniform(
            "mvp", vp * xySliceMatrix(_sliceIndices[XY]));
}

QMatrix4x4 HistSliceOrienView::yzSliceMatrix(int index) const
{
    QMatrix4x4 mat;
    mat.translate(index * 2.f / _dimHists[0], 0.f, 0.f);
    mat.translate(-1.f * (_dimHists[0] - 1) / _dimHists[0], 0.f, 0.f);
    mat.scale(1.f / _dimHists[0], 1.f, 1.f);
    return mat;
}

QMatrix4x4 HistSliceOrienView::xzSliceMatrix(int index) const
{
    QMatrix4x4 mat;
    mat.translate(0.f, index * 2.f / _dimHists[1], 0.f);
    mat.translate(0.f, -1.f * (_dimHists[1] - 1) / _dimHists[1], 0.f);
    mat.scale(1.f, 1.f / _dimHists[1], 1.f);
    return mat;
}

QMatrix4x4 HistSliceOrienView::xySliceMatrix(int index) const
{
    QMatrix4x4 mat;
    mat.translate(0.f, 0.f, index * 2.f / _dimHists[2]);
    mat.translate(0.f, 0.f, -1.f * (_dimHists[2] - 1) / _dimHists[2]);
    mat.scale(1.f, 1.f, 1.f / _dimHists[2]);
    return mat;
}
