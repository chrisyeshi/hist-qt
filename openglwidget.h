#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <opengl.h>
#include <QOpenGLWidget>
#include <functional>

/**
 * @brief The OpenGLWidget class
 */
class OpenGLWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit OpenGLWidget(QWidget* parent = 0)
      : QOpenGLWidget(parent)
      , _isInitialized(false) {}

protected:
    virtual void initializeGL() override {
#ifndef __APPLE__
        if(!gladLoadGL())
            throw "gladLoadGL failed!";
#endif
        _isInitialized = true;
        for (auto& initFunc : _initFuncs)
            initFunc();
        _initFuncs.clear();
    }
    void delayForInit(std::function<void()> func) {
        if (_isInitialized) {
            makeCurrent();
            func();
            doneCurrent();
        } else
            _initFuncs.push_back(func);
    }

protected:
    bool _isInitialized;
    std::vector<std::function<void()>> _initFuncs;
};

#endif // OPENGLWIDGET_H
