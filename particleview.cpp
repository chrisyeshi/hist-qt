#include "particleview.h"
#include <QBoxLayout>
#include <QMouseEvent>
#include <yygl/glerror.h>

ParticleView::ParticleView(QWidget *parent)
  : Widget(parent, Qt::Dialog)
  , _openglView(new ParticleOpenGLView(this))
{
    setMinimumSize(300, 300);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(_openglView);
}

void ParticleView::update()
{
    Widget::update();
    _openglView->update();
}

void ParticleView::setBoundingBox(glm::vec3 lower, glm::vec3 upper)
{
    _openglView->setBoundingBox(lower, upper);
}

void ParticleView::setParticles(const std::vector<Particle> *particles)
{
    _openglView->setParticles(particles);
}

/**
 * @brief ParticleOpenGLView::ParticleOpenGLView
 * @param parent
 */
ParticleOpenGLView::ParticleOpenGLView(QWidget *parent)
  : OpenGLWidget(parent)
  , _camera(std::make_shared<Camera>())
  , _boundingBoxLower(-1.f, -1.f, -1.f)
  , _boundingBoxUpper( 1.f,  1.f,  1.f)
{
    delayForInit(std::bind(&ParticleOpenGLView::initialize, this));
}

void ParticleOpenGLView::setBoundingBox(glm::vec3 lower, glm::vec3 upper)
{
    _boundingBoxLower = lower;
    _boundingBoxUpper = upper;
    delayForInit([this]() {
        makeCurrent();
        auto qBoundingBoxLower =
                QVector3D(_boundingBoxLower.x, _boundingBoxLower.y,
                    _boundingBoxLower.z);
        auto qBoundingBoxUpper =
                QVector3D(_boundingBoxUpper.x, _boundingBoxUpper.y,
                    _boundingBoxUpper.z);
        _camera->reset(qBoundingBoxLower, qBoundingBoxUpper);
        doneCurrent();
    });
    updateMVP();
}

void ParticleOpenGLView::setParticles(const std::vector<Particle> *particles)
{
    delayForInit([this, particles]() {
        makeCurrent();
        *_particles = *particles;
        _particlePass.setVBO("posAttr", _particles, 3, GL_DOUBLE, 0,
                sizeof(Particle), sizeof(int64_t) + sizeof(glm::dvec3));
        _particlePass.setVBO(
                "valAttr", _particles, 1, GL_DOUBLE, 0, sizeof(Particle),
                sizeof(int64_t) + 4 * sizeof(glm::dvec3) + sizeof(double));
        _particlePass.setFirstVertexIndex(0);
        _particlePass.setVertexCount(particles->size());
        doneCurrent();
    });
}

void ParticleOpenGLView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    _boundingBoxPass.drawElements();
    _particlePass.drawArrays();

    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_POINT_SPRITE);
    glDisable(GL_DEPTH_TEST);
}

void ParticleOpenGLView::resizeGL(int w, int h)
{
    _camera->setAspectRatio(float(w) / float(h));
    updateMVP();
}

void ParticleOpenGLView::mousePressEvent(QMouseEvent *event)
{
    _mousePrev = event->localPos();
}

void ParticleOpenGLView::mouseMoveEvent(QMouseEvent *event)
{
    auto mouseCurr = event->localPos();
    auto mouseDir =
            QVector2D(
                (mouseCurr - _mousePrev) / (height() * devicePixelRatio()));
    mouseDir.setY(-mouseDir.y());
    if (event->buttons() & Qt::LeftButton)
        _camera->orbit(mouseDir);
    if (event->buttons() & Qt::RightButton)
        _camera->track(mouseDir);
    // reset near and far plane
    auto qBoundingBoxLower =
            QVector3D(
                _boundingBoxLower.x, _boundingBoxLower.y, _boundingBoxLower.z);
    auto qBoundingBoxUpper =
            QVector3D(
                _boundingBoxUpper.x, _boundingBoxUpper.y, _boundingBoxUpper.z);
    _camera->resetNearFar(qBoundingBoxLower, qBoundingBoxUpper);
    updateMVP();
    _mousePrev = mouseCurr;
    update();
}

void ParticleOpenGLView::wheelEvent(QWheelEvent *event)
{
    auto numDegrees = float(event->angleDelta().y()) / 8.f;
    _camera->zoom(numDegrees);
    updateMVP();
    update();
}

void ParticleOpenGLView::initialize()
{
    glClearColor(1.f, 1.f, 1.f, 1.f);
    _particles = std::make_shared<yy::gl::buffer>();
    // bounding box render pass
    _boundingBoxPass.setProgram(
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
    _boundingBoxPass.setVBO<glm::vec3>("v_position", {
        { -1.f, -1.f, -1.f },
        {  1.f, -1.f, -1.f },
        { -1.f,  1.f, -1.f },
        {  1.f,  1.f, -1.f },
        { -1.f, -1.f,  1.f },
        {  1.f, -1.f,  1.f },
        { -1.f,  1.f,  1.f },
        {  1.f,  1.f,  1.f }
    });
    _boundingBoxPass.setIBO<GLuint>({
        0, 1, 0, 2, 2, 3, 1, 3, 4, 5, 4, 6, 6, 7, 5, 7, 0, 4, 1, 5, 2, 6, 3, 7
    });
    _boundingBoxPass.setDrawMode(yy::gl::render_pass::LINES);
    _boundingBoxPass.setFirstVertexIndex(0);
    _boundingBoxPass.setVertexCount(24);
    // particle pass
    _particlePass.setProgram(
            yy::gl::shader::VERTEX_SHADER,
            []() {
                QFile file(":/shaders/point.vert");
                file.open(QFile::ReadOnly);
                QTextStream in(&file);
                QString text = in.readAll();
                return text.toStdString();
            }(),
            yy::gl::shader::FRAGMENT_SHADER,
            []() {
                QFile file(":/shaders/point.frag");
                file.open(QFile::ReadOnly);
                QTextStream in(&file);
                QString text = in.readAll();
                return text.toStdString();
            }());
    _particlePass.setDrawMode(yy::gl::render_pass::POINTS);
    _particlePass.setFirstVertexIndex(0);
    _particlePass.setUniform("ptSize", 10.f);
}

void ParticleOpenGLView::updateMVP()
{
    delayForInit([this]() {
        makeCurrent();
        auto qBoundingBoxLower =
                QVector3D(_boundingBoxLower.x, _boundingBoxLower.y,
                    _boundingBoxLower.z);
        auto qBoundingBoxUpper =
                QVector3D(_boundingBoxUpper.x, _boundingBoxUpper.y,
                    _boundingBoxUpper.z);
        QMatrix4x4 model;
        model.translate(0.5f * (qBoundingBoxLower + qBoundingBoxUpper));
        model.scale(0.5f * (qBoundingBoxUpper - qBoundingBoxLower));
        _boundingBoxPass.setUniform(
                "mvp", _camera->matProj() * _camera->matView() * model);
        _particlePass.setUniform(
                "matVP", _camera->matProj() * _camera->matView());
        _particlePass.setUniform("matModel", QMatrix4x4());
        _particlePass.setUniform("campos", _camera->eye());
        doneCurrent();
    });
}
