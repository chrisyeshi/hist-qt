#ifndef PAINTER_H
#define PAINTER_H

#include <QImage>
#include <QPainter>
#include <yygl/glrenderpass.h>

class Painter
{
public:
    Painter();
    Painter(int w, int h);

public:
    void resize(int w, int h);
    void paint();

public:
    void save() { _painter->save(); }
    void restore() { _painter->restore(); }
    template <typename... Args>
    void setPen(Args... args) { _painter->setPen(args...); }
    template <typename... Args>
    void translate(Args... args) { _painter->translate(args...); }
    template <typename... Args>
    void rotate(Args... args) { _painter->rotate(args...); }
    template <typename... Args>
    void shear(Args... args) { _painter->shear(args...); }
    const QFont& font() const { return _painter->font(); }
    template <typename... Args>
    void setFont(Args... args) { _painter->setFont(args...); }

public:
    void clear();
    template <typename... Args>
    void drawLine(Args... args) { _painter->drawLine(args...); }
    template <typename... Args>
    void drawRect(Args... args) { _painter->drawRect(args...); }
    template <typename... Args>
    void fillRect(Args... args) { _painter->fillRect(args...); }
    template <typename... Args>
    void drawText(Args... args) { _painter->drawText(args...); }

private:
    std::shared_ptr<QImage> _image;
    std::shared_ptr<yy::gl::texture> _texture;
    std::shared_ptr<QPainter> _painter;
    yy::gl::render_pass _renderPass;
};

#endif // PAINTER_H
