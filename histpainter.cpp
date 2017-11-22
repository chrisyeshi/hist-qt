#include "histpainter.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <data/Histogram.h>

namespace {

std::shared_ptr<yy::gl::program> sharedBackgroundShaderProgram() {
    static auto program = std::make_shared<yy::gl::program>(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 viewport;
                in vec4 v_position;
                void main() {
                    vec2 np = v_position.xy * vec2(0.5) + vec2(0.5);
                    vec2 pos = np * viewport.zw + viewport.xy;
                    gl_Position = vec4(pos * vec2(2.0) - vec2(1.0), 0.0, 1.0);
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
    return program;
}

const glm::vec4 _yellowBlueBackgroundColor = {1.f, 1.f, 1.f, 1.f};

} // unnamed namespace

const glm::vec4 Hist1DVBOPainter::_yellowBlueBarColor = {
    51/255.f, 110/255.f, 123/255.f, 1.f
};
const glm::vec4 Hist1DVBOPainter::_grayScaleBackgroundColor = {
    200/255.0, 200/255.0, 200/255.0, 1.f
};
const glm::vec4 Hist1DVBOPainter::_grayScaleBarColor = {
    100/255.0, 100/255.0, 100/255.0, 1.f
};

/**
 * @brief Hist1DVBOPainter::initialize
 */
void Hist1DVBOPainter::initialize()
{
    // background render pass
    _backgroundRenderPass.setProgram(sharedBackgroundShaderProgram());
    _backgroundRenderPass.setVBO<glm::vec2>("v_position", {
        { -1.0, -1.0 },
        {  1.0, -1.0 },
        { -1.0,  1.0 },
        {  1.0,  1.0 }
    });
    _backgroundRenderPass.setDrawMode(yy::gl::render_pass::TRIANGLE_STRIP);
    _backgroundRenderPass.setFirstVertexIndex(0);
    _backgroundRenderPass.setVertexCount(4);
    _backgroundRenderPass.setUniforms("rect", glm::vec4(0.f, 0.f, 1.f, 1.f),
            "color", _yellowBlueBackgroundColor);
    // bars render pass
    _barsRenderPass.setProgram(sharedBarsShaderProgram());
    _barsRenderPass.setDrawMode(yy::gl::render_pass::POINTS);
    _barsRenderPass.setFirstVertexIndex(0);
    _barsRenderPass.setUniforms(
            "viewport", glm::vec4(0.f, 0.f, 1.f, 1.f),
            "rect", glm::vec4(0.f, 0.f, 1.f, 1.f), "vMin", 0.f, "vMax", 1.f,
            "color", _yellowBlueBarColor);
    // borders render pass
    _bordersRenderPass.setProgram(sharedTriangleStripBordersShaderProgram());
    _bordersRenderPass.setDrawMode(yy::gl::render_pass::POINTS);
    _bordersRenderPass.setFirstVertexIndex(0);
    _bordersRenderPass.setUniforms(
            "viewport", glm::vec4(0.f, 0.f, 1.f, 1.f),
            "rect", glm::vec4(0.f, 0.f, 1.f, 1.f), "vMin", 0.f, "vMax", 1.f,
            "thickness", 0.005f);
}

void Hist1DVBOPainter::paint()
{
    _backgroundRenderPass.drawArrays();
    _barsRenderPass.drawArrays();
    _bordersRenderPass.drawArrays();
}

void Hist1DVBOPainter::setNormalizedViewport(
        float x, float y, float w, float h) {
    _barsRenderPass.setUniform("viewport", glm::vec4(x, y, w, h));
    _backgroundRenderPass.setUniform("viewport", glm::vec4(x, y, w, h));
    _bordersRenderPass.setUniform("viewport", glm::vec4(x, y, w, h));
}

void Hist1DVBOPainter::setNormalizedRect(float x, float y, float w, float h) {
    _barsRenderPass.setUniform("rect", glm::vec4(x, y, w, h));
//    _backgroundRenderPass.setUniform("rect", glm::vec4(x, y, w, h));
    _bordersRenderPass.setUniform("rect", glm::vec4(x, y, w, h));
}

void Hist1DVBOPainter::setFreqRange(float min, float max)
{
    _barsRenderPass.setUniforms("vMin", min, "vMax", max);
    _bordersRenderPass.setUniforms("vMin", min, "vMax", max);
}

void Hist1DVBOPainter::setColorMap(IHistPainter::ColorMapOption option)
{
    if (IHistPainter::YELLOW_BLUE == option) {
        _backgroundRenderPass.setUniform("color", _yellowBlueBackgroundColor);
        _barsRenderPass.setUniform("color", _yellowBlueBarColor);
    } else if (IHistPainter::GRAY_SCALE == option) {
        _backgroundRenderPass.setUniform("color", _grayScaleBackgroundColor);
        _barsRenderPass.setUniform("color", _grayScaleBarColor);
    }
}

void Hist1DVBOPainter::setVBO(std::shared_ptr<yy::gl::vector<float>> histVBO)
{
    _barsRenderPass.setVBO("v_freq", *histVBO);
    _barsRenderPass.setUniform("nBins", float(histVBO->size()));
    _barsRenderPass.setVertexCount(histVBO->size());
    _bordersRenderPass.setVBO("v_freq", *histVBO);
    _bordersRenderPass.setUniform("nBins", float(histVBO->size()));
    _bordersRenderPass.setVertexCount(histVBO->size());
}

void Hist1DVBOPainter::setBackgroundColor(glm::vec4 color)
{
    _backgroundRenderPass.setUniform("color", color);
}

std::shared_ptr<yy::gl::program> Hist1DVBOPainter::sharedBarsShaderProgram()
{
    static auto program = std::make_shared<yy::gl::program>(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                in float v_freq;
                out float vg_freq;
                out int vg_vertexID;
                void main() {
                    gl_Position = vec4(v_freq, 0.0, 0.0, 1.0);
                    vg_freq = v_freq;
                    vg_vertexID = gl_VertexID;
                }
            )GLSL",
            yy::gl::shader::GEOMETRY_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 rect;
                uniform float vMin, vMax;
                uniform float nBins;
                layout(points) in;
                layout(triangle_strip, max_vertices = 4) out;
                in float vg_freq[];
                in int vg_vertexID[];
                out vec2 gf_position;
                void main() {
                    float normValue = (vg_freq[0] - vMin) / (vMax - vMin);
                    float w = rect.z * 2.0 / nBins;
                    float h = rect.w * 0.9 * normValue * 2.0;
                    float x = rect.x * 2.0 - 1.0 + vg_vertexID[0] * w;
                    float y = rect.y * 2.0 - 1.0;

                    gf_position = vec2(x, y);
                    gl_Position = vec4(gf_position, 0.0, 1.0);
                    EmitVertex();
                    gf_position = vec2(x + w, y);
                    gl_Position = vec4(gf_position, 0.0, 1.0);
                    EmitVertex();
                    gf_position = vec2(x, y + h);
                    gl_Position = vec4(gf_position, 0.0, 1.0);
                    EmitVertex();
                    gf_position = vec2(x + w, y + h);
                    gl_Position = vec4(gf_position, 0.0, 1.0);
                    EmitVertex();
                    EndPrimitive();
                }
            )GLSL",
            yy::gl::shader::FRAGMENT_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 color;
                uniform vec4 viewport;
                in vec2 gf_position;
                out vec4 f_color;
                void main() {
                    vec4 vp = vec4(viewport.xy * 2.0 - 1.0, viewport.zw * 2.0);
                    if (gf_position.x < vp.x || gf_position.x > vp.x + vp.z ||
                            gf_position.y < vp.y || gf_position.y > vp.y + vp.w)
                        discard;
                    f_color = color;
                }
            )GLSL");
    return program;
}

std::shared_ptr<yy::gl::program>
        Hist1DVBOPainter::sharedLineStripBordersShaderProgram() {
    static auto program = std::make_shared<yy::gl::program>(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                in float v_freq;
                out float vg_freq;
                out int vg_vertexID;
                void main() {
                    gl_Position = vec4(v_freq, 0.0, 0.0, 1.0);
                    vg_freq = v_freq;
                    vg_vertexID = gl_VertexID;
                }
            )GLSL",
            yy::gl::shader::GEOMETRY_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 rect;
                uniform float vMin, vMax;
                uniform float nBins;
                uniform float thickness;
                layout(points) in;
                layout(line_strip, max_vertices = 4) out;
                in float vg_freq[];
                in int vg_vertexID[];
                out vec2 gf_position;
                void main() {
                    float normValue = (vg_freq[0] - vMin) / (vMax - vMin);
                    float w = rect.z * 2.0 / nBins;
                    float h = rect.w * 0.9 * normValue * 2.0;
                    float x = rect.x * 2.0 - 1.0 + vg_vertexID[0] * w;
                    float y = rect.y * 2.0 - 1.0;

                    gf_position = vec2(x, y);
                    gl_Position = vec4(gf_position, 0.0, 1.0);
                    EmitVertex();
                    gf_position = vec2(x + w, y);
                    gl_Position = vec4(gf_position, 0.0, 1.0);
                    EmitVertex();
                    gf_position = vec2(x, y + h);
                    gl_Position = vec4(gf_position, 0.0, 1.0);
                    EmitVertex();
                    gf_position = vec2(x + w, y + h);
                    gl_Position = vec4(gf_position, 0.0, 1.0);
                    EmitVertex();
                    EndPrimitive();
                }
            )GLSL",
            yy::gl::shader::FRAGMENT_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 viewport;
                in vec2 gf_position;
                out vec4 f_color;
                void main() {
                    vec4 vp = vec4(viewport.xy * 2.0 - 1.0, viewport.zw * 2.0);
                    if (gf_position.x < vp.x || gf_position.x > vp.x + vp.z ||
                            gf_position.y < vp.y || gf_position.y > vp.y + vp.w)
                        discard;
                    f_color = vec4(0.2, 0.2, 0.2, 1.0);
                }
            )GLSL");
    return program;
}

std::shared_ptr<yy::gl::program>
        Hist1DVBOPainter::sharedTriangleStripBordersShaderProgram() {
    static auto program = std::make_shared<yy::gl::program>(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                in float v_freq;
                out float vg_freq;
                out int vg_vertexID;
                void main() {
                    gl_Position = vec4(v_freq, 0.0, 0.0, 1.0);
                    vg_freq = v_freq;
                    vg_vertexID = gl_VertexID;
                }
            )GLSL",
            yy::gl::shader::GEOMETRY_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 rect;
                uniform vec4 viewport;
                uniform float vMin, vMax;
                uniform float nBins;
                uniform float thickness;
                layout(points) in;
                layout(triangle_strip, max_vertices = 8) out;
                in float vg_freq[];
                in int vg_vertexID[];
                out vec2 gf_position;
                void main() {
                    float normValue = (vg_freq[0] - vMin) / (vMax - vMin);
                    float t = 0.5 * thickness * viewport.z;
                    float w = rect.z * 2.0 / nBins;
                    float h = max(t, rect.w * 0.9 * normValue * 2.0);
                    float x = rect.x * 2.0 - 1.0 + vg_vertexID[0] * w;
                    float y = rect.y * 2.0 - 1.0;

                    gf_position = vec2(x-t, y);
                    gl_Position = vec4(gf_position, 0.0, 1.0); EmitVertex();
                    gf_position = vec2(x+t, y);
                    gl_Position = vec4(gf_position, 0.0, 1.0); EmitVertex();
                    gf_position = vec2(x-t, y+h+t);
                    gl_Position = vec4(gf_position, 0.0, 1.0); EmitVertex();
                    gf_position = vec2(x+t, y+h-t);
                    gl_Position = vec4(gf_position, 0.0, 1.0); EmitVertex();
                    gf_position = vec2(x+w+t, y+h+t);
                    gl_Position = vec4(gf_position, 0.0, 1.0); EmitVertex();
                    gf_position = vec2(x+w-t, y+h-t);
                    gl_Position = vec4(gf_position, 0.0, 1.0); EmitVertex();
                    gf_position = vec2(x+w+t, y);
                    gl_Position = vec4(gf_position, 0.0, 1.0); EmitVertex();
                    gf_position = vec2(x+w-t, y);
                    gl_Position = vec4(gf_position, 0.0, 1.0); EmitVertex();
                    EndPrimitive();
                }
            )GLSL",
            yy::gl::shader::FRAGMENT_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 viewport;
                in vec2 gf_position;
                out vec4 f_color;
                void main() {
                    vec4 vp = vec4(viewport.xy * 2.0 - 1.0, viewport.zw * 2.0);
                    if (gf_position.x < vp.x || gf_position.x > vp.x + vp.z ||
                            gf_position.y < vp.y || gf_position.y > vp.y + vp.w)
                        discard;
                    f_color = vec4(0.2, 0.2, 0.2, 1.0);
                }
            )GLSL");
    return program;
}

/**
 * @brief Hist2DTexturePainter::initialize
 * This must be called after the OpenGL context is initialized.
 */
void Hist2DTexturePainter::initialize()
{
    // background render pass
    _backgroundRenderPass.setProgram(sharedBackgroundShaderProgram());
    _backgroundRenderPass.setVBO<glm::vec2>("v_position", {
        { -1.0, -1.0 },
        {  1.0, -1.0 },
        { -1.0,  1.0 },
        {  1.0,  1.0 }
    });
    _backgroundRenderPass.setDrawMode(yy::gl::render_pass::TRIANGLE_STRIP);
    _backgroundRenderPass.setFirstVertexIndex(0);
    _backgroundRenderPass.setVertexCount(4);
    _backgroundRenderPass.setUniforms("rect", glm::vec4(0.f, 0.f, 1.f, 1.f),
            "color", _yellowBlueBackgroundColor);
    // heatmap render pass
    _heatmapRenderPass.setProgram(sharedShaderProgram());
    _heatmapRenderPass.setVBO<glm::vec2>("v_position", {
        { -1.0, -1.0 },
        {  1.0, -1.0 },
        { -1.0,  1.0 },
        {  1.0,  1.0 }
    });
    _heatmapRenderPass.setDrawMode(yy::gl::render_pass::TRIANGLE_STRIP);
    _heatmapRenderPass.setFirstVertexIndex(0);
    _heatmapRenderPass.setVertexCount(4);
    _heatmapRenderPass.setUniforms(
            "rect", glm::vec4(0.f, 0.f, 1.f, 1.f), "viewport",
            glm::vec4(0.f, 0.f, 1.f, 1.f), "vMin", 0.f, "vMax", 1.f);
}

void Hist2DTexturePainter::setTexture(
        std::shared_ptr<yy::gl::texture> histTexture)
{
    _heatmapRenderPass.setTexture(
            histTexture, "tex", 0, yy::gl::texture::TEXTURE_2D);
}

void Hist2DTexturePainter::setNormalizedViewport(
        float x, float y, float w, float h) {
    _backgroundRenderPass.setUniform("viewport", glm::vec4(x, y, w, h));
    _heatmapRenderPass.setUniform("viewport", glm::vec4(x, y, w, h));
}

void Hist2DTexturePainter::setNormalizedRect(
        float x, float y, float w, float h) {
    _heatmapRenderPass.setUniform("rect", glm::vec4(x, y, w, h));
}

void Hist2DTexturePainter::setFreqRange(float min, float max)
{
    _heatmapRenderPass.setUniforms("vMin", min, "vMax", max);
}

void Hist2DTexturePainter::setColorMap(IHistPainter::ColorMapOption option)
{
    if (IHistPainter::YELLOW_BLUE == option) {
        _heatmapRenderPass.setTexture(
                sharedYellowBlueColorMapTexture(), "colormap", 1,
                yy::gl::texture::TEXTURE_1D);
    } else if (IHistPainter::GRAY_SCALE == option) {
        _heatmapRenderPass.setTexture(
                sharedGrayScaleColorMapTexture(), "colormap", 1,
                yy::gl::texture::TEXTURE_1D);
    }
}

void Hist2DTexturePainter::paint()
{
    _backgroundRenderPass.drawArrays();
    _heatmapRenderPass.drawArrays();
}

std::shared_ptr<yy::gl::program> Hist2DTexturePainter::sharedShaderProgram()
{
    static auto program = std::make_shared<yy::gl::program>(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 rect;
                in vec4 v_position;
                out vec2 vf_texCoord;
                out vec2 vf_position;
                void main() {
                    vec2 np = v_position.xy * vec2(0.5) + vec2(0.5);
                    vec2 pos = np * rect.zw + rect.xy;
                    vf_position = pos * vec2(2.0) - vec2(1.0);
                    gl_Position = vec4(vf_position, 0.0, 1.0);
                    vf_texCoord = v_position.xy * 0.5 + 0.5;
                }
            )GLSL",
            yy::gl::shader::FRAGMENT_SHADER,
            R"GLSL(
                #version 330
                uniform sampler2D tex;
                uniform sampler1D colormap;
                uniform vec4 viewport;
                uniform float vMin;
                uniform float vMax;
                in vec2 vf_texCoord;
                in vec2 vf_position;
                out vec4 f_color;
                void main() {
                    vec4 vp = vec4(viewport.xy * 2.0 - 1.0, viewport.zw * 2.0);
                    if (vf_position.x < vp.x || vf_position.x > vp.x + vp.z ||
                            vf_position.y < vp.y || vf_position.y > vp.y + vp.w)
                        discard;
                    float val = texture(tex, vf_texCoord).r;
                    float freq = log(val - vMin + 1) / log(vMax - vMin + 1);
                    f_color = texture(colormap, freq);
                }
            )GLSL");
    return program;
}

std::shared_ptr<yy::gl::texture>
        Hist2DTexturePainter::sharedGrayScaleColorMapTexture() {
    static auto texture = []() {
        std::shared_ptr<yy::gl::texture> texture =
                std::make_shared<yy::gl::texture>();
        texture->setWrapMode(
                yy::gl::texture::TEXTURE_1D, yy::gl::texture::CLAMP_TO_EDGE);
        texture->setTextureMinMagFilter(yy::gl::texture::TEXTURE_1D,
                yy::gl::texture::MIN_LINEAR, yy::gl::texture::MAG_LINAER);
        texture->texImage1D<glm::vec3>(
                yy::gl::texture::TEXTURE_1D, yy::gl::texture::INTERNAL_RGB32F,
                yy::gl::texture::FORMAT_RGB, {
            { 200/255.0, 200/255.0, 200/255.0 },
            { 100/255.0, 100/255.0, 100/255.0 }
        });
        return texture;
    }();
    return texture;
}

std::shared_ptr<yy::gl::texture>
        Hist2DTexturePainter::sharedYellowBlueColorMapTexture() {
    static auto texture = []() {
        std::shared_ptr<yy::gl::texture> texture =
                std::make_shared<yy::gl::texture>();
        texture->setWrapMode(
                yy::gl::texture::TEXTURE_1D, yy::gl::texture::CLAMP_TO_EDGE);
        texture->setTextureMinMagFilter(yy::gl::texture::TEXTURE_1D,
                yy::gl::texture::MIN_LINEAR, yy::gl::texture::MAG_LINAER);
        texture->texImage1D<glm::vec3>(
                yy::gl::texture::TEXTURE_1D, yy::gl::texture::INTERNAL_RGB32F,
                yy::gl::texture::FORMAT_RGB, ColormapPresets::yellowBlue());
        return texture;
    }();
    return texture;
}

/**
 * @brief Hist2DPainter::initialize
 */
void Hist2DPainter::initialize()
{
    // hist 2d texture painter
    _painter.initialize();
    // texture
    std::vector<float> frequencies(_hist2d->nBins());
    for (auto iBin = 0; iBin < _hist2d->nBins(); ++iBin) {
        frequencies[iBin] = _hist2d->values()[iBin];
    }
    auto texture = std::make_shared<yy::gl::texture>();
    texture->setWrapMode(
            yy::gl::texture::TEXTURE_2D, yy::gl::texture::CLAMP_TO_EDGE);
    texture->setTextureMinMagFilter(yy::gl::texture::TEXTURE_2D,
            yy::gl::texture::MIN_NEAREST, yy::gl::texture::MAG_NEAREST);
    texture->texImage2D(
            yy::gl::texture::TEXTURE_2D, yy::gl::texture::INTERNAL_R32F,
            _hist2d->dim()[0], _hist2d->dim()[1], yy::gl::texture::FORMAT_RED,
            yy::gl::texture::FLOAT, frequencies.data());
    _painter.setTexture(texture);
}

void Hist2DPainter::paint()
{
    _painter.paint();
}

void Hist2DPainter::setNormalizedViewport(float x, float y, float w, float h) {
    _painter.setNormalizedViewport(x, y, w, h);
}

void Hist2DPainter::setNormalizedRect(float x, float y, float w, float h) {
    _painter.setNormalizedRect(x, y, w, h);
}

void Hist2DPainter::setFreqRange(float min, float max)
{
    _painter.setFreqRange(min, max);
}

void Hist2DPainter::setColorMap(IHistPainter::ColorMapOption option)
{
    _painter.setColorMap(option);
}
