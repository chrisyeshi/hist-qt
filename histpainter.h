#ifndef HISTPAINTER_H
#define HISTPAINTER_H

#include <memory>
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

/**
 * @brief The IHistPainter class
 */
class IHistPainter
{
public:
    enum ColorMapOption {
        GRAY_SCALE,
        YELLOW_BLUE
    };

public:
    virtual ~IHistPainter() {}

public:
    virtual void initialize() = 0;
    virtual void paint() = 0;
    virtual void setRect(float x, float y, float w, float h) = 0;
    virtual void setRange(float min, float max) = 0;
    virtual void setColorMap(ColorMapOption option) = 0;
};

/**
 * @brief The Hist1DVBOPainter class
 */
class Hist1DVBOPainter : public IHistPainter {
public:
    virtual void initialize() override;
    virtual void paint() override;
    virtual void setRect(float x, float y, float w, float h) override;
    virtual void setRange(float min, float max) override;
    virtual void setColorMap(ColorMapOption option) override;

public:
    void setVBO(std::shared_ptr<yy::gl::vector<float>> histVBO);
    void setBackgroundColor(glm::vec4 color);

private:
    static std::shared_ptr<yy::gl::program> sharedBackgroundShaderProgram();
    static std::shared_ptr<yy::gl::program> sharedBarsShaderProgram();
    static std::shared_ptr<yy::gl::program> sharedBordersShaderProgram();

private:
    yy::gl::render_pass _backgroundRenderPass;
    yy::gl::render_pass _barsRenderPass;
    yy::gl::render_pass _bordersRenderPass;

private:
    static const glm::vec4 _yellowBlueBackgroundColor;
    static const glm::vec4 _yellowBlueBarColor;
    static const glm::vec4 _grayScaleBackgroundColor;
    static const glm::vec4 _grayScaleBarColor;
};

/**
 * @brief The Hist2DTexturePainter class
 */
class Hist2DTexturePainter : public IHistPainter {
public:
    virtual void initialize() override;
    virtual void paint() override;
    virtual void setRect(float x, float y, float w, float h) override;
    virtual void setRange(float min, float max) override;
    virtual void setColorMap(ColorMapOption option) override;

public:
    void setTexture(std::shared_ptr<yy::gl::texture> histTexture);

private:
    static std::shared_ptr<yy::gl::program> sharedShaderProgram();
    static std::shared_ptr<yy::gl::texture> sharedGrayScaleColorMapTexture();
    static std::shared_ptr<yy::gl::texture> sharedYellowBlueColorMapTexture();

private:
    yy::gl::render_pass _renderPass;
};

/**
 * @brief The Hist2DPainter class
 */
class Hist2DPainter : public IHistPainter {
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
    static std::shared_ptr<yy::gl::texture> sharedGrayScaleColorMapTexture();
    static std::shared_ptr<yy::gl::texture> sharedYellowBlueColorMapTexture();

private:
    const Hist2D* _hist2d;
    Hist2DTexturePainter _painter;
};

#endif // HISTPAINTER_H
