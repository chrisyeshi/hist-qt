#ifndef PARTICLEVIEW_H
#define PARTICLEVIEW_H

#include <widget.h>
#include <yygl/glrenderpass.h>
#include <openglwidget.h>
#include <functional>
#include <data/tracerreader.h>
#include <camera.h>

class ParticleOpenGLView;

/**
 * @brief The ParticleView class
 */
class ParticleView : public Widget
{
    Q_OBJECT
public:
    explicit ParticleView(QWidget *parent = 0);

signals:

public:
    void update();
    void setBoundingBox(glm::vec3 lower, glm::vec3 upper);
    void setParticles(const std::vector<Particle>* particles);

private:
    ParticleOpenGLView* _openglView;
};

/**
 * @brief The ParticleOpenGLView class
 */
class ParticleOpenGLView : public OpenGLWidget {
    Q_OBJECT
public:
    explicit ParticleOpenGLView(QWidget* parent = 0);

signals:

public:
    void setBoundingBox(glm::vec3 lower, glm::vec3 upper);
    void setParticles(const std::vector<Particle>* particles);

protected:
    virtual void paintGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;

private:
    void initialize();
    void updateMVP();

private:
    yy::gl::render_pass _boundingBoxPass;
    yy::gl::render_pass _particlePass;
    std::shared_ptr<yy::gl::buffer> _particles;
    std::shared_ptr<Camera> _camera;
    glm::vec3 _boundingBoxLower, _boundingBoxUpper;
    QPointF _mousePrev;
};

#endif // PARTICLEVIEW_H
