#ifndef HISTVOLUMEPHYSICALVIEW_H
#define HISTVOLUMEPHYSICALVIEW_H

#include <histvolumeview.h>
#include <openglwidget.h>
#include <yygl/gltexture.h>
#include <volren/volume/volumegl.h>
#include <volren/volren.h>
#include <camera.h>

class HistView;
class HistVolumePhysicalOpenGLView;
class HistFacadePainter;

/**
 * @brief The HistVolumePhysicalView class
 */
class HistVolumePhysicalView : public HistVolumeView {
public:
    HistVolumePhysicalView(QWidget* parent = nullptr);

public:
    virtual void update() override;
    virtual void setHistConfigs(std::vector<HistConfig> configs) override;
    virtual void setDataStep(std::shared_ptr<DataStep> dataStep) override;

private:
    std::string currHistName() const;

private:
//    HistView* _histView;
    HistVolumePhysicalOpenGLView* _histVolumeView;
    std::vector<HistConfig> _histConfigs;
    std::vector<int> _histDims;
    std::shared_ptr<DataStep> _dataStep;
    int _currHistId = 0;
    int _currHistConfigId = 0;
};

/**
 * @brief The HistVolumePhysicalOpenGLView class
 */
class HistVolumePhysicalOpenGLView : public OpenGLWidget {
    Q_OBJECT
public:
    enum Orien : int {XY = 2, XZ = 1, YZ = 0};

public:
    HistVolumePhysicalOpenGLView(QWidget* parent = nullptr);

public:
    void setHistVolume(HistConfig histConfig,
            std::shared_ptr<HistFacadeVolume> histVolume);

protected:
    void resizeGL(int w, int h);
    void paintGL();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    QRectF calcDefaultSliceRect() const;
    QRectF calcSliceRect() const;
    void boundSliceTransform();
    void updateCurrSlice();
    void createHistPainters();
    void setHistsToHistPainters();
    void setRangesToHistPainters();
    void updateHistPainterRects();
    void updateSliceIdScrollBar();
    int getSliceCountInDirection(Extent dimHists, Orien orien);

private:
    const float _defaultZoom = 1.f;
//    const float _border = 0.1f;
    const float _borderPixel = 10.0f;
    static constexpr QVector2D _defaultTranslate = QVector2D(0.5f, 0.5f);
    const std::vector<int> _defaultDims = {0};
    static const Orien _defaultOrien = XY;
    static const int _defaultSliceId = 0;

private:
    std::shared_ptr<HistFacadeVolume> _histVolume = nullptr;
    std::vector<int> _currDims = _defaultDims;
    Orien _currOrien = _defaultOrien;
    int _currSliceId = _defaultSliceId;
    std::shared_ptr<HistFacadeRect> _currSlice;
    float _currZoom = _defaultZoom;
    QVector2D _currTranslate = _defaultTranslate;
    QPointF _mousePrev;
    std::vector<std::shared_ptr<HistFacadePainter>> _histPainters;

    //    std::vector<std::shared_ptr<yy::VolumeGL>> _avgVolumes;
    //    std::unique_ptr<yy::volren::VolRen> _volren;
    //    std::shared_ptr<Camera> _camera;
};

#endif // HISTVOLUMEPHYSICALVIEW_H
