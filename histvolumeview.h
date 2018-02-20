#ifndef HISTVOLUMEVIEW_H
#define HISTVOLUMEVIEW_H

#include <widget.h>
#include <data/DataPool.h>

/**
 * @brief The HistVolumeView class
 */
class HistVolumeView : public Widget {
public:
    HistVolumeView(QWidget* parent = nullptr) : Widget(parent) {}
    virtual ~HistVolumeView() {}
    virtual void update() = 0;
    virtual void setHistConfigs(std::vector<HistConfig> configs) = 0;
    virtual void setDataStep(std::shared_ptr<DataStep> dataStep) = 0;
    typedef std::map<int, std::array<double, 2>> HistRangesMap;
    virtual void setCustomHistRanges(const HistRangesMap& histRangesMap) = 0;
    virtual void setCurrHistVolume(const QString& histVolumeName) = 0;
};

/**
 * @brief The HistVolumeNullView class
 */
class HistVolumeNullView : public HistVolumeView {
public:
    HistVolumeNullView(QWidget* parent = nullptr) : HistVolumeView(parent) {}
    virtual void update() override {}
    virtual void setHistConfigs(std::vector<HistConfig>) override {}
    virtual void setDataStep(std::shared_ptr<DataStep>) override {}
    virtual void setCustomHistRanges(
            const HistRangesMap& histRangesMap) override {}
    virtual void setCurrHistVolume(const QString& histVolumeName) override {}
};

#endif // HISTVOLUMEVIEW_H
