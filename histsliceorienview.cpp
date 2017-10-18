#include "histsliceorienview.h"
#include <camera.h>
#include <QMouseEvent>

HistSliceOrienView::HistSliceOrienView(QWidget *parent)
  : OpenGLWidget(parent)
  , _dimHists(2, 2, 2)
  , _sliceIndices({{ 0, 0, 0 }})
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
  , _camera(new Camera()) {
    initialize();
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

void HistSliceOrienView::setHoveredHist(
        std::array<int, 3> histIds, bool hovered)
{
    makeCurrent();
    _histHovered = hovered;
    _hoveredHistIds = histIds;
    auto vp = _camera->matProj() * _camera->matView();
    _hoveredRenderPass.setUniform(
            "mvp", vp * hoveredHistMatrix(histIds));
    doneCurrent();
    update();
}

void HistSliceOrienView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    _boundingBoxRenderPass.drawElements();
    for (auto& pass : _highlightRenderPasses)
        pass.drawElements();
    if (_histHovered)
        _hoveredRenderPass.drawElements();
    glDisable(GL_LINE_SMOOTH);
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
    _mousePrev = event->localPos();
}

void HistSliceOrienView::mouseMoveEvent(QMouseEvent *event)
{
    auto mouseCurr = event->localPos();
    auto mouseDir =
            QVector2D(
                (mouseCurr - _mousePrev) / (height() * devicePixelRatio()));
    mouseDir.setY(-mouseDir.y());
    if (event->buttons() & Qt::LeftButton)
        _camera->orbit(mouseDir);
    // reset near and far plane
    _camera->resetNearFar({ -1.f, -1.f, -1.f }, { 1.f, 1.f, 1.f });
    updateMVP();
    _mousePrev = mouseCurr;
    update();
}

void HistSliceOrienView::initialize() {
    delayForInit([this]() {
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
                        f_color = vec4(0.25, 0.25, 0.25, 1.0);
                    }
                )GLSL");
        _boundingBoxRenderPass.setVBO<glm::vec3>("v_position", _cube);
        _boundingBoxRenderPass.setIBO<GLuint>({
            0, 1, 0, 2, 2, 3, 1, 3, 4, 5, 4, 6,
            6, 7, 5, 7, 0, 4, 1, 5, 2, 6, 3, 7
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
                    out vec3 vg_position;
                    void main() {
                        gl_Position = mvp * v_position;
                        vg_position = v_position.xyz;
                    }
                )GLSL",
                yy::gl::shader::GEOMETRY_SHADER,
                R"GLSL(
                    #version 330
                    layout(triangles) in;
                    layout(triangle_strip, max_vertices = 3) out;
                    uniform mat3 nm;
                    in vec3 vg_position[];
                    out vec3 gf_normal;
                    void main() {
                        vec3 a = vg_position[0];
                        vec3 b = vg_position[1];
                        vec3 c = vg_position[2];
                        vec3 normal = nm * normalize(cross(b - a, c - a));
                        for (int i = 0; i < 3; ++i) {
                            gl_Position = gl_in[i].gl_Position;
                            gf_normal = normal;
                            EmitVertex();
                        }
                        EndPrimitive();
                    }
                )GLSL",
                yy::gl::shader::FRAGMENT_SHADER,
                R"GLSL(
                    #version 330
                    uniform vec4 color;
                    in vec3 gf_normal;
                    out vec4 f_color;
                    void main() {
                        vec3 normal = normalize(gf_normal);
                        float df = abs(dot(normal, vec3(0.0, 0.0, 1.0)));
                        vec3 ambient = 0.4 * color.rgb;
                        vec3 diffuse = 1.6 * color.rgb * df;
                        f_color = vec4(ambient + diffuse, color.a);
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
        /// TODO: this is the same program as the bounding box render pass.
        // hovered render pass
        _hoveredRenderPass.setProgram(
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
                        f_color = vec4(0.25, 0.25, 0.25, 0.5);
                    }
                )GLSL");
        _hoveredRenderPass.setVBO<glm::vec3>("v_position", _cube);
        _hoveredRenderPass.setIBO<GLuint>({
            0, 1, 0, 2, 2, 3, 1, 3, 4, 5, 4, 6,
            6, 7, 5, 7, 0, 4, 1, 5, 2, 6, 3, 7
        });
        _hoveredRenderPass.setDrawMode(yy::gl::render_pass::LINES);
        _hoveredRenderPass.setFirstVertexIndex(0);
        _hoveredRenderPass.setVertexCount(24);
        // camera
        _camera->reset({ -1.f, -1.f, -1.f }, { 1.f, 1.f, 1.f });
        _camera->setFOV(_camera->fov() * 0.9);
        updateMVP();
    });
}

void HistSliceOrienView::updateMVP()
{
    delayForInit([this]() {
        auto vp = _camera->matProj() * _camera->matView();
        _boundingBoxRenderPass.setUniform("mvp", vp);
        auto yzMVP = vp * yzSliceMatrix(_sliceIndices[YZ]);
        _highlightRenderPasses[YZ].setUniforms(
                "mvp", yzMVP, "nm", yzMVP.normalMatrix());
        auto xzMVP = vp * xzSliceMatrix(_sliceIndices[XZ]);
        _highlightRenderPasses[XZ].setUniforms(
                "mvp", xzMVP, "nm", xzMVP.normalMatrix());
        auto xyMVP = vp * xySliceMatrix(_sliceIndices[XY]);
        _highlightRenderPasses[XY].setUniforms(
                "mvp", xyMVP, "nm", xyMVP.normalMatrix());
    });
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

QMatrix4x4 HistSliceOrienView::hoveredHistMatrix(std::array<int, 3> histIds)
{
    QMatrix4x4 mat;
    float w = 1.f / _dimHists[0];
    float h = 1.f / _dimHists[1];
    float d = 1.f / _dimHists[2];
    mat.translate(
            histIds[0] * 2.f * w, histIds[1] * 2.f * h, histIds[2] * 2.f * d);
    mat.translate(w - 1.f, h - 1.f, d - 1.f);
    mat.scale(w, h, d);
    return mat;
}
