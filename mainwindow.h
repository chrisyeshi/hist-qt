#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <data/DataPool.h>
#include <queryview.h>
#include <mask.h>

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
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void open();
    void open(const QString& dir);
    void toggleQueryView(bool show);
    void toggleTimelineView(bool show);
    void toggleParticleView(bool show);
    void exportParticles();
    void setTimeStep(int timeStep);
    void setRules(const std::vector<QueryRule>& rules);
    void applyQueryRules();
    std::vector<Particle> loadTracers(int timeStep, const BoolMask3D& mask);

private:
    unsigned int nHist() const;

private:
    HistVolumeView* _histVolumeView;
    HistCompareView* _histCompareView;
    QueryView* _queryView;
    TimelineView* _timelineView;
    ParticleView* _particleView;

private:
    QPushButton* _queryViewToggleButton;
    QPushButton* _particleViewToggleButton;
    QPushButton* _timelineViewToggleButton;

private:
    DataPool _data;
    int _currTimeStep;
    std::vector<QueryRule> _queryRules;
    BoolMask3D _selectedHistMask;
    std::vector<Particle> _particles;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
