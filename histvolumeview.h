#ifndef HISTVOLUMEVIEW_H
#define HISTVOLUMEVIEW_H

#include <widget.h>
#include <array>
#include <QMap>
#include <openglwidget.h>
#include <data/DataPool.h>
#include <yygl/glrenderpass.h>
#include <mask.h>

class QSlider;
class QComboBox;
class QScrollBar;
class QStackedLayout;
class QStackedWidget;
class HistVolumeViewImpl;
class HistPainter;
class Hist2DView;
class HistSliceView;
class HistSliceOrienView;
class Camera;

/**
 * @brief The HistVolumeView class
 */
class HistVolumeView : public Widget
{
    Q_OBJECT
public:
    explicit HistVolumeView(QWidget *parent = 0);

public:
    enum Layout : int { SLICE = 0, LIST = 1 };
    void update();
    void setHistConfigs(std::vector<HistConfig> configs);
    void setDataStep(std::shared_ptr<DataStep> dataStep);
    void setSelectedHistMask(BoolMask3D selectedHistMask);

private:
    void setLayout(Layout layout);
    void selectHistVolume(const QString& name);
    void updateHistDimensions(const HistConfig& histConfig);
    void selectHistDimension(const QString& dimStr);
    HistVolumeViewImpl* currentImpl() const;

private:
    QComboBox* _layoutCombo;
    QComboBox* _histVolumeCombo;
    QComboBox* _histDimensionCombo;
    QMap<QString, std::vector<int>> _histDimStrToIndVec;
    QStackedLayout* _stackedLayout;
    QVector<HistVolumeViewImpl*> _impls;
    std::shared_ptr<DataStep> _dataStep;
    std::vector<HistConfig> _histConfigs;
    QString _histName;
    std::vector<int> _histDims;
    BoolMask3D _selectedHistMask;
};

/// TODO: pull the following classes into separate files before implementing the
/// list view.
/**
 * @brief The HistVolumeViewImpl class
 */
class HistVolumeViewImpl {
public:
    virtual ~HistVolumeViewImpl() {}
    virtual void setHistVolume(
            std::shared_ptr<const HistFacadeVolume> histVolume) = 0;
    virtual void setHistDimensions(const std::vector<int>& dims) = 0;
    virtual void setSelectedHistMask(BoolMask3D mask) = 0;
    virtual void update() = 0;
};

/**
 * @brief The HistVolumeViewSlice class
 */
class HistVolumeViewSlice : public QWidget, public HistVolumeViewImpl {
    Q_OBJECT
public:
    explicit HistVolumeViewSlice(QWidget* parent);

public:
    virtual void setHistVolume(
            std::shared_ptr<const HistFacadeVolume> histVolume) override;
    virtual void setHistDimensions(const std::vector<int>& dims) override;
    virtual void setSelectedHistMask(BoolMask3D mask) override;
    virtual void update() override;

private:
    static const int NUM_SLICES = 3;
    static const int YZ = 0;
    static const int XZ = 1;
    static const int XY = 2;

private:
    void updateSliceViews();
    void setXYSliceIndex(int index);
    void setXZSliceIndex(int index);
    void setYZSliceIndex(int index);
    void setCurrHistFromXYSlice(
            std::array<int, 2> rectIds, std::vector<int> dims);
    void setCurrHistFromXZSlice(
            std::array<int, 2> rectIds, std::vector<int> dims);
    void setCurrHistFromYZSlice(
            std::array<int, 2> rectIds, std::vector<int> dims);
    void setCurrHist(std::array<int, 3> histIds, std::vector<int> dims);

private:
    QVector<QScrollBar*> _sliceIndexScrollBars;
    QVector<HistSliceView*> _sliceViews;
    QStackedWidget* _histOrienWidget;
    HistSliceOrienView* _orienView;
    Hist2DView* _histView;
    std::shared_ptr<const HistFacadeVolume> _histVolume;
    BoolMask3D _selectedHistMask;
    std::vector<int> _histDims;
    int _xySliceIndex, _xzSliceIndex, _yzSliceIndex;
    std::shared_ptr<const HistFacade> _currHist;
};

#endif // HISTVOLUMEVIEW_H
