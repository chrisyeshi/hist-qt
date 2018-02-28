#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <data/DataPool.h>
#include <queryview.h>

class HistView;
class HistVolumeView;
class HistCompareView;
class QueryView;
class ParticleView;
class TimelineView;
class QComboBox;
class QPushButton;
class QStackedLayout;
class UserStudyCtrlView;

class HistViewHolder : public QWidget {
    Q_OBJECT
public:
    HistViewHolder(QWidget* parent = nullptr);
    typedef std::map<int, std::array<double, 2>> HistRangesMap;

signals:
    void selectedHistRangesChanged(HistRangesMap histRangesMap);

public:
    void update();
    void setHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);
    void setCustomVarRanges(const HistRangesMap &varRangesMap);
    void setText(const QString& text);
    void showText();
    void showHist();

public:
    virtual int heightForWidth(int w) const override { return w; }
    virtual bool hasHeightForWidth() const override { return true; }

private:
    QStackedLayout* _layout;
    HistView* _histView;
    QLabel* _label;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(
            const std::string& layout = "particle", QWidget *parent = 0);
    ~MainWindow();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private:
    void createParticleLayout();
    void createSimpleLayout();
    void createUserStudyLayout();

private:
    void open();
    void open(const QString& dir);
    void toggleQueryView(bool show);
    void toggleTimelineView(bool show);
    void toggleParticleView(bool show);
    void exportParticles();
    void setTimeStep(int timeStep);
    void setRules(const std::vector<QueryRule>& rules);
    std::vector<Particle> loadTracers(
            int timeStep, const std::vector<int>& selectedHistFlatIds);
    void readSettings();

private:
    void toWelcomePage();
    void toUserStudyBgPage();
    void toTutorialPage();
    void toUserStudyPage(int pageId);

private:
    HistVolumeView* _histVolumeView;
    HistCompareView* _histCompareView;
    QueryView* _queryView;
    TimelineView* _timelineView;
    ParticleView* _particleView;
    HistViewHolder* _histView;
    QStackedLayout* _sLayout;
    UserStudyCtrlView* _userStudyCtrlView;

private:
    QPushButton* _queryViewToggleButton;
    QPushButton* _particleViewToggleButton;
    QPushButton* _timelineViewToggleButton;

private:
    DataPool _data;
    DataPool::Stats _dataStats;
    int _currTimeStep;
    std::vector<Particle> _particles;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
