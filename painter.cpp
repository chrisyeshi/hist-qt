#include "painter.h"

Painter::Painter() : Painter(100, 100) {}

Painter::Painter(int w, int h)
  : _image(new QImage(w, h, QImage::Format_RGBA8888))
  , _texture(new yy::gl::texture())
  , _painter(new QPainter(_image.get())) {
    _image->fill(QColor(0, 0, 0, 0));
    _painter->setRenderHint(QPainter::Antialiasing);
    _renderPass.setProgram(
            yy::gl::shader::VERTEX_SHADER,
            R"GLSL(
                #version 330
                in vec4 v_position;
                out vec2 vf_texCoord;
                void main() {
                    gl_Position = v_position;
                    vf_texCoord = (v_position.xy + 1.0) * 0.5;
                }
            )GLSL",
            yy::gl::shader::FRAGMENT_SHADER,
            R"GLSL(
                #version 330
                uniform sampler2D tex;
                in vec2 vf_texCoord;
                out vec4 f_color;
                void main() {
                    f_color = texture(tex, vf_texCoord);
                }
            )GLSL");
    _texture->setTextureMinMagFilter(yy::gl::texture::TEXTURE_2D,
            yy::gl::texture::MIN_LINEAR, yy::gl::texture::MAG_LINAER);
    _renderPass.setTexture(_texture, "tex", 0, yy::gl::texture::TEXTURE_2D);
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

Painter::Painter(QPaintDevice *device)
  : _painter(new QPainter(device))
  , _image(nullptr)
  , _texture(nullptr) {
    _painter->setRenderHint(QPainter::Antialiasing);
}

void Painter::resize(int w, int h)
{
    if (!_image) {
        return;
    }
    _image = std::make_shared<QImage>(w, h, QImage::Format_RGBA8888);
    _image->fill(QColor(0, 0, 0, 0));
    _painter = std::make_shared<QPainter>(_image.get());
    _painter->setRenderHint(QPainter::Antialiasing);
}

void Painter::paint() {
    if (!_image) {
        return;
    }
    _texture->texImage2D(
            yy::gl::texture::TEXTURE_2D, yy::gl::texture::INTERNAL_RGBA8,
            _image->width(), _image->height(), yy::gl::texture::FORMAT_RGBA,
            yy::gl::texture::UNSIGNED_BYTE, _image->mirrored().bits());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _renderPass.drawArrays();
    glDisable(GL_BLEND);
}
