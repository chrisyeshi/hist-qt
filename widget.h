#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class Widget : public QWidget {
    Q_OBJECT
public:
    explicit Widget(
            QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags())
      : QWidget(parent, f) {}

signals:
    void visibilityChanged(bool);

protected:
    virtual void showEvent(QShowEvent*) override {
        emit visibilityChanged(true);
    }
    virtual void hideEvent(QHideEvent*) override {
        emit visibilityChanged(false);
    }
};

#endif // WIDGET_H
