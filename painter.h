#ifndef PAINTER_H
#define PAINTER_H

#include <QImage>
#include <QPainter>
#include <yygl/glrenderpass.h>

/**
 * @brief The PainterImpl class
 */
class PainterImpl {
public:
    virtual bool begin(QPainter* painter, QPaintDevice* paintDevice) {
        bool ret = painter->begin(paintDevice);
        painter->setRenderHints(
                QPainter::Antialiasing | QPainter::TextAntialiasing |
                QPainter::SmoothPixmapTransform);
        return ret;
    }
    virtual bool end(QPainter* painter) { return painter->end(); }
    virtual void resize(QPainter*, int, int) {}
};

/**
 * @brief The PainterYYGLImpl class
 */
class PainterYYGLImpl : public PainterImpl {
public:
    virtual bool begin(QPainter *painter, QPaintDevice *paintDevice) {
        // setup render pass except texture
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
        _renderPass.setVBO<glm::vec2>("v_position", {
            { -1.0, -1.0 },
            {  1.0, -1.0 },
            { -1.0,  1.0 },
            {  1.0,  1.0 }
        });
        _renderPass.setDrawMode(yy::gl::render_pass::TRIANGLE_STRIP);
        _renderPass.setFirstVertexIndex(0);
        _renderPass.setVertexCount(4);
        // enlarge the QImage size for high resolution displays
        int sw = paintDevice->width() * paintDevice->devicePixelRatio();
        int sh = paintDevice->height() * paintDevice->devicePixelRatio();
        if (!_image || _image->width() != sw || _image->height() != sh) {
            _image = std::make_shared<QImage>(sw, sh, QImage::Format_RGBA8888);
            _image->setDevicePixelRatio(paintDevice->devicePixelRatio());
            _image->fill(QColor(0, 0, 0, 0));
            _texture = std::make_shared<yy::gl::texture>();
            _texture->setTextureMinMagFilter(yy::gl::texture::TEXTURE_2D,
                    yy::gl::texture::MIN_LINEAR, yy::gl::texture::MAG_LINAER);
            _renderPass.setTexture(
                    _texture, "tex", 0, yy::gl::texture::TEXTURE_2D);
        }
        // setup QPainter to use QImage as device
        if (!painter->begin(_image.get())) {
            return false;
        }
        painter->setRenderHints(
                QPainter::Antialiasing | QPainter::TextAntialiasing |
                QPainter::SmoothPixmapTransform);
        return true;
    }
    virtual bool end(QPainter* painter) {
        auto image = _image->mirrored();
        _texture->texImage2D(
                yy::gl::texture::TEXTURE_2D, yy::gl::texture::INTERNAL_RGBA8,
                _image->width(), _image->height(), yy::gl::texture::FORMAT_RGBA,
                yy::gl::texture::UNSIGNED_BYTE, image.bits());
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        _renderPass.drawArrays();
        glDisable(GL_BLEND);
        return painter->end();
    }
    virtual void resize(QPainter* /*painter*/, int /*w*/, int /*h*/) {
        /// TODO: allocate memory for QImage outside for paintGL()
    }

private:
    std::shared_ptr<QImage> _image;
    std::shared_ptr<yy::gl::texture> _texture;
    yy::gl::render_pass _renderPass;
};

/**
 * @brief The PainterImageImpl class
 */
class PainterImageImpl : public PainterImpl {
public:
    virtual bool begin(QPainter *painter, QPaintDevice *paintDevice) {
        _paintDevice = paintDevice;
        // enlarge the QImage size for high resolution displays
        int sw = paintDevice->width() * paintDevice->devicePixelRatio();
        int sh = paintDevice->height() * paintDevice->devicePixelRatio();
        if (!_image || _image->width() != sw || _image->height() != sh) {
            _image = std::make_shared<QImage>(sw, sh, QImage::Format_RGBA8888);
            _image->setDevicePixelRatio(paintDevice->devicePixelRatio());
            _image->fill(QColor(0, 0, 0, 0));
        }
        // setup QPainter to use QImage as device
        if (!painter->begin(_image.get())) {
            return false;
        }
        painter->setRenderHints(
                QPainter::Antialiasing | QPainter::TextAntialiasing |
                QPainter::SmoothPixmapTransform);
        return true;
    }
    virtual bool end(QPainter* painter) {
        assert(_paintDevice);
        QPainter p(_paintDevice);
        p.drawImage(
                QRectF(0, 0, _image->width() / _image->devicePixelRatioF(),
                    _image->height() / _image->devicePixelRatioF()),
                *_image);
        return painter->end();
    }
    virtual void resize(QPainter* /*painter*/, int /*w*/, int /*h*/) {
        /// TODO: allocate memory for QImage outside for paintGL()
    }

private:
    std::shared_ptr<QImage> _image;
    QPaintDevice* _paintDevice = nullptr;
};

/**
 * @brief The Painter class
 */
class Painter {
public:
    Painter() : Painter(nullptr) {}
    Painter(QPaintDevice* paintDevice)
          : Painter(std::make_shared<PainterImpl>(), paintDevice) {}
    Painter(std::shared_ptr<PainterImpl> impl,
            QPaintDevice* paintDevice = nullptr)
          : _impl(impl) {
        if (paintDevice) begin(paintDevice);
    }
    ~Painter() { end(); }

public:
    bool begin(QPaintDevice* paintDevice) {
        return _impl->begin(&_painter, paintDevice);
    }
    bool end() { return _impl->end(&_painter); }
    void resize(int w, int h) { _impl->resize(&_painter, w, h); }

public:
    void save() { _painter.save(); }
    void restore() { _painter.restore(); }
    template <typename... Args>
    void setPen(Args... args) { _painter.setPen(args...); }
    template <typename... Args>
    void translate(Args... args) { _painter.translate(args...); }
    template <typename... Args>
    void rotate(Args... args) { _painter.rotate(args...); }
    template <typename... Args>
    void shear(Args... args) { _painter.shear(args...); }
    const QFont& font() const { return _painter.font(); }
    template <typename... Args>
    void setFont(Args... args) { _painter.setFont(args...); }

public:
    template <typename... Args>
    QRect boundingRect(Args... args) { return _painter.boundingRect(args...); }
    template <typename... Args>
    void drawLine(Args... args) { _painter.drawLine(args...); }
    template <typename... Args>
    void drawRect(Args... args) { _painter.drawRect(args...); }
    template <typename... Args>
    void fillRect(Args... args) { _painter.fillRect(args...); }
    template <typename... Args>
    void drawText(Args... args) { _painter.drawText(args...); }

private:
    QPainter _painter;
    std::shared_ptr<PainterImpl> _impl;
};

#endif // PAINTER_H
