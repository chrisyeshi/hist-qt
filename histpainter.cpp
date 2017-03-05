#include "histpainter.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <data/Histogram.h>

void Hist2DPainter::initialize()
{
    initializeOpenGLFunctions();
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
    // render pass
    _renderPass.setProgram(sharedShaderProgram());
    _renderPass.setTexture(texture, "tex", 0, yy::gl::texture::TEXTURE_2D);
    _renderPass.setTexture(sharedYellowBlueColorMapTexture(), "colormap", 1,
            yy::gl::texture::TEXTURE_1D);
    _renderPass.setVBO<glm::vec2>("v_position", {
        { -1.0, -1.0 },
        {  1.0, -1.0 },
        { -1.0,  1.0 },
        {  1.0,  1.0 }
    });
    _renderPass.setDrawMode(yy::gl::render_pass::TRIANGLE_STRIP);
    _renderPass.setFirstVertexIndex(0);
    _renderPass.setVertexCount(4);
}

void Hist2DPainter::paint()
{
    _renderPass.drawArrays();
}

void Hist2DPainter::setRect(float x, float y, float w, float h)
{
    _left = x;
    _bottom = y;
    _width = w;
    _height = h;
    _renderPass.setUniforms("rect", glm::vec4(_left, _bottom, _width, _height));
}

void Hist2DPainter::setRange(float min, float max)
{
    _min = min;
    _max = max;
    _renderPass.setUniforms("vMin", _min, "vMax", _max);
}

void Hist2DPainter::setColorMap(HistPainter::ColorMapOption option)
{
    if (option == GRAY_SCALE) {
        _renderPass.setTexture(sharedGrayScaleColorMapTexture(), "colormap", 1,
                yy::gl::texture::TEXTURE_1D);
    } else if (option == YELLOW_BLUE) {
        _renderPass.setTexture(sharedYellowBlueColorMapTexture(), "colormap",
                1, yy::gl::texture::TEXTURE_1D);
    }
}

std::shared_ptr<yy::gl::program> Hist2DPainter::sharedShaderProgram()
{
    static auto program = std::make_shared<yy::gl::program>(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                uniform vec4 rect;
                in vec4 v_position;
                out vec2 vf_texCoord;
                void main() {
                    vec2 np = v_position.xy * vec2(0.5) + vec2(0.5);
                    vec2 pos = np * rect.zw + rect.xy;
                    gl_Position = vec4(pos * vec2(2.0) - vec2(1.0), 0.0, 1.0);
                    vf_texCoord = v_position.xy * 0.5 + 0.5;
                }
            )GLSL",
            yy::gl::shader::FRAGMENT_SHADER,
            R"GLSL(
                #version 330
                uniform sampler2D tex;
                uniform sampler1D colormap;
                uniform float vMin;
                uniform float vMax;
                in vec2 vf_texCoord;
                out vec4 f_color;
                void main() {
                    float val = texture(tex, vf_texCoord).r;
                    float freq = (val - vMin) / (vMax - vMin);
                    f_color = texture(colormap, freq);
                }
            )GLSL");
    return program;
}

std::shared_ptr<yy::gl::texture> Hist2DPainter::sharedGrayScaleColorMapTexture()
{
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

std::shared_ptr<yy::gl::texture> Hist2DPainter::sharedYellowBlueColorMapTexture()
{
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
            { 255/255.0, 255/255.0, 204/255.0 },
            { 161/255.0, 218/255.0, 180/255.0 },
            { 65/255.0,  182/255.0, 196/255.0 },
            { 44/255.0,  127/255.0, 184/255.0 },
            { 37/255.0,  52/255.0,  148/255.0 }
        });
        return texture;
    }();
    return texture;
}
