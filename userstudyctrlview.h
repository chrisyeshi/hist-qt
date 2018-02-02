#ifndef USERSTUDYCTRLVIEW_H
#define USERSTUDYCTRLVIEW_H

#include <QWidget>

namespace Ui {
class UserStudyCtrlView;
}

class UserStudyCtrlView : public QWidget {
    Q_OBJECT
public:
    explicit UserStudyCtrlView(QWidget *parent = 0);
    ~UserStudyCtrlView();

signals:
    void toPage(int pageId);

public:
    void setCurrentPage(int pageId);

private:
    void nextButtonClicked();

private:
    Ui::UserStudyCtrlView *ui;
    int _pageId = 0;
};

#endif // USERSTUDYCTRLVIEW_H
