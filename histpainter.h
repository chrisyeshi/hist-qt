#ifndef HISTPAINTER_H
#define HISTPAINTER_H

#include <memory>
#include <QOpenGLFunctions_3_3_Core>
#include <yygl/glbuffer.h>
#include <yygl/glvector.h>
#include <yygl/glvertexarray.h>
#include <yygl/glshader.h>
#include <yygl/gltexture.h>
#include <yygl/glrenderpass.h>

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class Hist2D;

class HistPainter
{
public:
    enum ColorMapOption {
        GRAY_SCALE,
        YELLOW_BLUE
    };

public:
    virtual ~HistPainter() {}

public:
    virtual void initialize() = 0;
    virtual void paint() = 0;
    virtual void setRect(float x, float y, float w, float h) = 0;
    virtual void setRange(float min, float max) = 0;
    virtual void setColorMap(ColorMapOption option) = 0;
};

class Hist2DPainter : public HistPainter, protected QOpenGLFunctions_3_3_Core {
public:
    /// TODO: change to setHist instead of setting the hist in constructor.
    Hist2DPainter(const Hist2D* hist2d) : _hist2d(hist2d) {}

public:
    virtual void initialize() override;
    virtual void paint() override;
    virtual void setRect(float x, float y, float w, float h) override;
    virtual void setRange(float min, float max) override;
    virtual void setColorMap(ColorMapOption option) override;

private:
    static std::shared_ptr<yy::gl::program> sharedShaderProgram();
    static std::shared_ptr<yy::gl::texture> sharedGrayScaleColorMapTexture();
    static std::shared_ptr<yy::gl::texture> sharedYellowBlueColorMapTexture();

private:
    const Hist2D* _hist2d;
    yy::gl::render_pass _renderPass;
    yy::gl::texture _texture;
    yy::gl::program _program;
    yy::gl::vertex_array _vao;
    yy::gl::vector<glm::vec2> _vbo;
    float _left, _bottom, _width, _height;
    float _min, _max;
};

#endif // HISTPAINTER_H
