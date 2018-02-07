#ifndef HISTVOLUMESLICEVIEW_H
#define HISTVOLUMESLICEVIEW_H

#include <histvolumeview.h>
#include <array>
#include <QMap>
#include <set>
#include <openglwidget.h>
#include <yygl/glrenderpass.h>

class QSlider;
class QComboBox;
class QScrollBar;
class QStackedLayout;
class QStackedWidget;
class IHistVolumeSliceViewImpl;
class HistPainter;
class HistView;
class Hist2DView;
class HistSliceView;
class HistSliceOrienView;
class HistFacadeCollectionView;
class HistDimsCombo;
class Camera;

/**
 * @brief The HistVolumeSliceView class
 */
class HistVolumeSliceView : public HistVolumeView {
    Q_OBJECT
public:
    explicit HistVolumeSliceView(QWidget *parent = 0);

public:
    virtual void update() override;
    virtual void setHistConfigs(std::vector<HistConfig> configs) override;
    virtual void setDataStep(std::shared_ptr<DataStep> dataStep) override;
    virtual void setCustomHistRanges(
            const HistRangesMap& histRangesMap) override {}
    virtual void setCurrHistVolume(const QString& histVolumeName) override {
        selectHistVolume(histVolumeName);
    }

private:
    enum Layout : int { SLICE = 0, LIST = 1 };
    void setLayout(Layout layout);
    void selectHistVolume(const QString& name);
    void selectHistDimension(const QString& dimStr);
    void selectHistDims(std::vector<int> dims);
    IHistVolumeSliceViewImpl* currentImpl() const;
    void repaintSliceViews();
    void popHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);

private:
    QComboBox* _layoutCombo;
    QComboBox* _histVolumeCombo;
    HistDimsCombo* _histDimsCombo;

    QComboBox* _histDimensionCombo;
    QMap<QString, std::vector<int>> _histDimStrToIndVec;

    QStackedLayout* _stackedLayout;
    QVector<IHistVolumeSliceViewImpl*> _impls;
    std::shared_ptr<DataStep> _dataStep;
    std::vector<HistConfig> _histConfigs;
    QString _histName;
    std::vector<int> _histDims;
    HistFacadeCollectionView* _histFacadeCollectionView;
};

/**
 * @brief The HistVolumeSliceViewImpl class
 */
class IHistVolumeSliceViewImpl {
public:
    virtual ~IHistVolumeSliceViewImpl() {}
    virtual void setHistVolume(
            std::shared_ptr<const HistFacadeVolume> histVolume) = 0;
    virtual void setHistDimensions(const std::vector<int>& dims) = 0;
    virtual void update() = 0;
    virtual void repaintSliceViews() = 0;
};

/**
 * @brief The HistVolumeViewSlice class
 */
class HistVolumeSliceViewImpl : public QWidget, public IHistVolumeSliceViewImpl
{
    Q_OBJECT
public:
    explicit HistVolumeSliceViewImpl(QWidget* parent);

public:
    virtual void setHistVolume(
            std::shared_ptr<const HistFacadeVolume> histVolume) override;
    virtual void setHistDimensions(const std::vector<int>& dims) override;
    virtual void update() override;
    virtual void repaintSliceViews() override;

signals:
    void popHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);

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
    void unsetCurrHist();
    void setHoveredHistFromXYSlice(
            std::array<int, 2> rectIds, std::vector<int> dims, bool hovered);
    void setHoveredHistFromXZSlice(
            std::array<int, 2> rectIds, std::vector<int> dims, bool hovered);
    void setHoveredHistFromYZSlice(
            std::array<int, 2> rectIds, std::vector<int> dims, bool hovered);
    void setHoveredHist(
            std::array<int, 3> histIds, std::vector<int>, bool hovered);
    void multiSelectHistFromXYSlice(
            std::array<int,2> rectIds, std::vector<int> dims);
    void multiSelectHistFromXZSlice(
            std::array<int,2> rectIds, std::vector<int> dims);
    void multiSelectHistFromYZSlice(
            std::array<int,2> rectIds, std::vector<int> dims);
    void multiSelectHist(std::array<int,3> histIds, std::vector<int> dims);
    void updateCurrHist(std::vector<int> dims);
    void updateSliceViewsMultiHists();
    void setCurrentOrienWidget(QString text);

private:
    QVector<QScrollBar*> _sliceIndexScrollBars;
    QVector<HistSliceView*> _sliceViews;
    QStackedWidget* _histOrienWidget;
    QComboBox* _orienCombo;
    HistSliceOrienView* _orienView;
    HistView* _histView;
    std::shared_ptr<const HistFacadeVolume> _histVolume;
    std::vector<int> _histDims;
    int _xySliceIndex, _xzSliceIndex, _yzSliceIndex;
    std::shared_ptr<const HistFacade> _currHist;
    std::set<std::array<int,3>> _multiHistIds;
};

#endif // HISTVOLUMESLICEVIEW_H
