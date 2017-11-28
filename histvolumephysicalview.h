#ifndef HISTVOLUMEPHYSICALVIEW_H
#define HISTVOLUMEPHYSICALVIEW_H

#include <histvolumeview.h>
#include <openglwidget.h>
#include <QOpenGLFramebufferObject>
#include <yygl/gltexture.h>
#include <yygl/glrenderpass.h>
#include <volren/volume/volumegl.h>
#include <volren/volren.h>
#include <camera.h>

class HistView;
class HistVolumePhysicalOpenGLView;
class HistFacadePainter;
class QGestureEvent;

/**
 * @brief The HistVolumePhysicalView class
 */
class HistVolumePhysicalView : public HistVolumeView {
    Q_OBJECT
public:
    HistVolumePhysicalView(QWidget* parent = nullptr);

signals:
    void selectedHistsChanged(std::vector<std::shared_ptr<const Hist>>);

public:
    virtual void update() override;
    virtual void setHistConfigs(std::vector<HistConfig> configs) override;
    virtual void setDataStep(std::shared_ptr<DataStep> dataStep) override;
    virtual void setCustomHistRanges(
            const HistRangesMap& histRangesMap) override;

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
    enum Orien : int {YZ = 0, XZ = 1, XY = 2};
    enum NormalizedPer : int {
        NormPer_Histogram, NormPer_HistSlice, NormPer_HistVolume, NormPer_Custom
    };

public:
    HistVolumePhysicalOpenGLView(QWidget* parent = nullptr);

signals:
    void selectedHistsChanged(std::vector<std::shared_ptr<const Hist>>);

public:
    void setHistVolume(HistConfig histConfig,
            std::shared_ptr<HistFacadeVolume> histVolume);
    typedef std::map<int, std::array<float, 2>> HistRangesMap;
    void setCustomHistRanges(const HistRangesMap& histRangesMap);

protected:
    void resizeGL(int w, int h);
    void paintGL();
    bool event(QEvent* event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);

private:
    void render();
    int sw() const { return width(); }
    int sh() const { return height(); }
    int swf() const { return width(); }
    int shf() const { return height(); }
    QRectF calcDefaultSliceRect() const;
    QRectF calcSliceRect(float zoom, QVector2D translate) const;
    QRectF calcSliceRect() const;
    QRectF calcHistRect(std::array<int, 2> histSliceIds);
    float calcMaxZoom(const QRectF& defaultSliceRect) const;
    float calcMaxZoom() const;
    void boundSliceTransform();
    void updateCurrSlice();
    void createHistPainters();
    void setHistsToHistPainters();
    std::array<float, 2> calcFreqRange() const;
    void setFreqRangesToHistPainters(const std::array<float, 2>& range);
    void setFreqRangesToHistPainters();
    std::vector<std::array<double, 2>> calcHistRanges() const;
    void setHistRangesToHistPainters(
            const std::vector<std::array<double, 2>>& ranges);
    void setHistRangesToHistPainters();
    void updateHistPainterRects();
    void updateSliceIdScrollBar();
    int getSliceCountInDirection(Extent dimHists, Orien orien) const;
    std::array<int, 2> localPositionToHistSliceId(QPointF localPos) const;
    bool isHistSliceIdsValid(std::array<int, 2> histSliceIds) const;
    QColor quarterColor() const;
    QColor fullColor() const;
    void emitSelectedHistsChanged();
    std::vector<std::array<int, 2>> filterByCurrSlice(
            const std::vector<std::array<int, 3>>& histIds);
    std::array<int, 3> sliceIdsToHistIds(std::array<int, 2> histSliceIds);
    void translateEvent(const QVector2D& delta);
    void zoomEvent(float scale, const QVector2D& pos);
    std::shared_ptr<QOpenGLFramebufferObject> createWidgetSizeFbo() const;

private:
    const float _clickDelta = 5.f;
    const float _defaultZoom = 1.f;
    const float _borderPixel = 10.0f;
    const float _histSpacing = 1.f;
    const QColor _spacingColor = QColor(255, 100, 100);
    const QVector2D _defaultTranslate = QVector2D(0.5f, 0.5f);
    const std::vector<int> _defaultDims = {0};
    static const Orien _defaultOrien = XY;
    static const NormalizedPer _defaultFreqNormPer = NormPer_Histogram;
    static const NormalizedPer _defaultHistNormPer = NormPer_Histogram;
    static const int _defaultSliceId = 0;

private:
    HistConfig _histConfig;
    std::shared_ptr<HistFacadeVolume> _histVolume = nullptr;
    std::vector<int> _currDims = _defaultDims;
    Orien _currOrien = _defaultOrien;
    int _currSliceId = _defaultSliceId;
    std::shared_ptr<HistFacadeRect> _currSlice;
    float _currZoom = _defaultZoom;
    QVector2D _currTranslate = _defaultTranslate;
    QPointF _mousePrev, _mousePress;
    std::vector<std::shared_ptr<HistFacadePainter>> _histPainters;
    std::shared_ptr<QOpenGLFramebufferObject> _histSliceFbo;
    std::array<int, 2> _hoveredHistSliceIds = {{-1, -1}};
    std::vector<std::array<int, 3>> _selectedHistIds;
    NormalizedPer _currFreqNormPer = _defaultFreqNormPer;
    std::array<float, 2> _currFreqRange;
    NormalizedPer _currHistNormPer = _defaultHistNormPer;
    std::vector<std::array<double, 2>> _currHistRanges
            = {{NAN, NAN}, {NAN, NAN}};

    //    std::vector<std::shared_ptr<yy::VolumeGL>> _avgVolumes;
    //    std::unique_ptr<yy::volren::VolRen> _volren;
    //    std::shared_ptr<Camera> _camera;
};

Q_DECLARE_METATYPE(std::vector<std::shared_ptr<const Hist>>)

#endif // HISTVOLUMEPHYSICALVIEW_H
