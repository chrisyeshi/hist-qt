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

class ColormapPresets {
public:
    static std::vector<glm::vec3> yellowBlue() {
        return {
            { 255/255.0, 255/255.0, 204/255.0 },
            { 161/255.0, 218/255.0, 180/255.0 },
            { 65/255.0,  182/255.0, 196/255.0 },
            { 44/255.0,  127/255.0, 184/255.0 },
            { 37/255.0,  52/255.0,  148/255.0 }
        };
    }
};

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
    virtual void setNormalizedViewport(float x, float y, float w, float h) = 0;
    virtual void setNormalizedRect(float x, float y, float w, float h) = 0;
    virtual void setFreqRange(float min, float max) = 0;
    virtual void setColorMap(ColorMapOption option) = 0;

public:
    void setNormalizedViewportAndRect(float x, float y, float w, float h) {
        setNormalizedViewport(x, y, w, h);
        setNormalizedRect(x, y, w, h);
    }
};

/**
 * @brief The HistNullPainter class
 */
class HistNullPainter : public IHistPainter {
public:
    virtual void initialize() override {}
    virtual void paint() override {}
    virtual void setNormalizedViewport(
            float x, float y, float w, float h) override {}
    virtual void setNormalizedRect(
            float x, float y, float w, float h) override {}
    virtual void setFreqRange(float min, float max) override {}
    virtual void setColorMap(ColorMapOption option) override {}
};

/**
 * @brief The Hist1DVBOPainter class
 */
class Hist1DVBOPainter : public IHistPainter {
public:
    virtual void initialize() override;
    virtual void paint() override;
    virtual void setNormalizedViewport(
            float x, float y, float w, float h) override;
    virtual void setNormalizedRect(float x, float y, float w, float h) override;
    virtual void setFreqRange(float min, float max) override;
    virtual void setColorMap(ColorMapOption option) override;

public:
    void setVBO(std::shared_ptr<yy::gl::vector<float>> histVBO);
    void setBackgroundColor(glm::vec4 color);

private:
    static std::shared_ptr<yy::gl::program> sharedBarsShaderProgram();
    static std::shared_ptr<yy::gl::program>
            sharedLineStripBordersShaderProgram();
    static std::shared_ptr<yy::gl::program>
            sharedTriangleStripBordersShaderProgram();

private:
    yy::gl::render_pass _backgroundRenderPass;
    yy::gl::render_pass _barsRenderPass;
    yy::gl::render_pass _bordersRenderPass;

private:
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
    virtual void setNormalizedViewport(
            float x, float y, float w, float h) override;
    virtual void setNormalizedRect(float x, float y, float w, float h) override;
    virtual void setFreqRange(float min, float max) override;
    virtual void setColorMap(ColorMapOption option) override;

public:
    void setTexture(std::shared_ptr<yy::gl::texture> histTexture);

private:
    static std::shared_ptr<yy::gl::program> sharedShaderProgram();
    static std::shared_ptr<yy::gl::texture> sharedGrayScaleColorMapTexture();
    static std::shared_ptr<yy::gl::texture> sharedYellowBlueColorMapTexture();

private:
    yy::gl::render_pass _backgroundRenderPass;
    yy::gl::render_pass _heatmapRenderPass;
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
    virtual void setNormalizedViewport(
            float x, float y, float w, float h) override;
    virtual void setNormalizedRect(float x, float y, float w, float h) override;
    virtual void setFreqRange(float min, float max) override;
    virtual void setColorMap(ColorMapOption option) override;

private:
    static std::shared_ptr<yy::gl::texture> sharedGrayScaleColorMapTexture();
    static std::shared_ptr<yy::gl::texture> sharedYellowBlueColorMapTexture();

private:
    const Hist2D* _hist2d;
    Hist2DTexturePainter _painter;
};

#endif // HISTPAINTER_H
