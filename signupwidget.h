#ifndef SIGNUPWIDGET_H
#define SIGNUPWIDGET_H

#include <QWidget>

namespace Ui {
class SignUpWidget;
}

class SignUpWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SignUpWidget(QWidget *parent = 0);
    ~SignUpWidget();

signals:
    void submit();
    void cancel();

private:
    void clear();

private:
    Ui::SignUpWidget *ui;
};

#endif // SIGNUPWIDGET_H
