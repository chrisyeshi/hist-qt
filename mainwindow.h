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
    unsigned int nHist() const;

private:
    HistVolumeView* _histVolumeView;
    HistCompareView* _histCompareView;
    QueryView* _queryView;
    TimelineView* _timelineView;
    ParticleView* _particleView;
    HistView* _histView;

private:
    QPushButton* _queryViewToggleButton;
    QPushButton* _particleViewToggleButton;
    QPushButton* _timelineViewToggleButton;

private:
    DataPool _data;
    int _currTimeStep;
    std::vector<Particle> _particles;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
