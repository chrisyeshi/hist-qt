#ifndef LAZYUI_H
#define LAZYUI_H

#include <QMap>
#include <QSet>
#include <QLabel>
#include <QWidget>
#include <QComboBox>
#include <QBoxLayout>
#include <QScrollBar>
#include <QPushButton>

/**
 * @brief The LabeledWidget class
 */
template <typename T>
class LabeledWidget : public QWidget {
public:
    LabeledWidget(QString text = tr(""), QWidget* parent = nullptr)
          : QWidget(parent) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);
        QLabel* label = new QLabel(text ,this);
        label->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
        layout->addWidget(label);
        _widget = new T(this);
        layout->addWidget(_widget);
    }

public:
    T* widget() const { return _widget; }

private:
    T* _widget = nullptr;
};

/**
 * @brief The LazyPanel class
 */
class LazyPanel : public QWidget {
    Q_OBJECT
public:
    LazyPanel(QWidget* parent = nullptr) : QWidget(parent) {
        _layout = new QVBoxLayout(this);
        _layout->setMargin(0);
        _layout->setSpacing(5);
        _layout->addStretch(1);
    }

public:
    void addWidget(QWidget* widget) {
        _layout->insertWidget(_layout->count() - 1, widget);
    }

private:
    QVBoxLayout* _layout = nullptr;
};

/**
 * @brief The LazyUI class
 */
class LazyUI : public QObject {
// singleton
public:
    static LazyUI& instance() {
        static LazyUI lazyUI;
        return lazyUI;
    }
    ~LazyUI() {
        if (!_destroyed)
            _panel->deleteLater();
    }

public:
    LazyPanel* panel() const { return _panel; }
    bool exists(const QString& key) {
        return _widgets.contains(key);
    }
    void button(const QString& name, QObject* context,
            std::function<void()> func) {
        if (!_widgets.contains(name)) {
            auto button = new QPushButton(name);
            _panel->addWidget(button);
            _widgets[name] = button;
        }
        QPushButton* button = static_cast<QPushButton*>(_widgets[name]);
        button->disconnect(SIGNAL(clicked()));
        connect(button, &QPushButton::clicked, context, func);
    }
    void labeledButton(const QString& label, const QString& buttonText,
            QObject* context, std::function<void()> func) {
        if (!_widgets.contains(buttonText)) {
            auto labeledButton = new LabeledWidget<QPushButton>(label);
            labeledButton->widget()->setText(buttonText);
            _panel->addWidget(labeledButton);
            _widgets[buttonText] = labeledButton;
        }
        auto labeledButton =
                static_cast<LabeledWidget<QPushButton>*>(_widgets[buttonText]);
        auto button = labeledButton->widget();
        button->disconnect(SIGNAL(clicked()));
        connect(button, &QPushButton::clicked, context, func);
    }
    void labeledCombo(QString key, const QString& label,
            const QStringList& items, QObject* context,
            std::function<void(const QString&)> func) {
        if (!_widgets.contains(key)) {
            auto labeledCombo = new LabeledWidget<QComboBox>(label);
            _panel->addWidget(labeledCombo);
            _widgets[key] = labeledCombo;
        }
        auto combo =
                static_cast<LabeledWidget<QComboBox>*>(_widgets[key])->widget();
        combo->blockSignals(true);
        combo->clear();
        combo->addItems(items);
        combo->blockSignals(false);
        combo->disconnect(SIGNAL(currentTextChanged(const QString&)));
        connect(combo, &QComboBox::currentTextChanged, context, func);
    }
    void labeledScrollBar(QString key, const QString& label, int minimum,
            int maximum, int value, QObject* context,
            std::function<void(int)> func) {
        if (!_widgets.contains(key)) {
            auto labeledScrollBar = new LabeledWidget<QScrollBar>(label);
            labeledScrollBar->widget()->setOrientation(Qt::Horizontal);
            _panel->addWidget(labeledScrollBar);
            _widgets[key] = labeledScrollBar;
        }
        auto labeledScrollBar =
                static_cast<LabeledWidget<QScrollBar>*>(_widgets[key]);
        auto scrollBar = labeledScrollBar->widget();
        scrollBar->blockSignals(true);
        scrollBar->setPageStep(1);
        scrollBar->setRange(minimum, maximum);
        scrollBar->setValue(value);
        scrollBar->blockSignals(false);
        scrollBar->disconnect(SIGNAL(valueChanged(int)));
        connect(scrollBar, &QScrollBar::valueChanged, context, func);
    }
    template <typename Type>
    Type value(const QString& name) {
        if (!_widgets.contains(name)) {

        }
    }

private:
    LazyPanel* _panel = nullptr;
    bool _destroyed = false;
    QMap<QString, QWidget*> _widgets;

// deleted constructors
public:
    LazyUI(const LazyUI&) = delete;
    LazyUI& operator=(const LazyUI&) = delete;

private:
    LazyUI() {
        _panel = new LazyPanel();
        connect(_panel, &LazyPanel::destroyed, [this]() {
            _destroyed = true;
        });
    }
};

#endif // LAZYUI_H
