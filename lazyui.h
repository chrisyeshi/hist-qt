#ifndef LAZYUI_H
#define LAZYUI_H

#include <QMap>
#include <QSet>
#include <QLabel>
#include <QtMath>
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QBoxLayout>
#include <QScrollBar>
#include <QPushButton>
#include <cassert>

/**
 * @brief The FluidLayout class
 */
class FluidLayout : public QLayout {
public:
    struct Item {
        enum Size {Minimum, Small, Medium, Large, FullLine};
        Item() = default;
        Item(QLayoutItem* layoutItem, Size size = Medium)
              : layoutItem(layoutItem), size(size) {}
        operator bool () const { return layoutItem; }
        QLayoutItem* layoutItem = nullptr;
        Size size;
    };

public:
    explicit FluidLayout(QWidget* parent = nullptr) : QLayout(parent) {}
    ~FluidLayout() {
        Item item;
        while ((item = takeAt(0)))
            delete item.layoutItem;
    }

public:
    virtual bool hasHeightForWidth() const override { return true; }
    virtual int heightForWidth(int w) const override {
        const QRect effectiveRect(
                geometry().x(), geometry().y(), w, geometry().height());
        int iLine = 0, iCell = 0;
        for (Item itemHolder : _items) {
            QLayoutItem* item = itemHolder.layoutItem;
            int nCells =
                    getNumberOfCells(effectiveRect.size(), itemHolder.size);
            if (iCell + nCells <= nCellsPerLine) {
                iCell += nCells;
            } else {
                ++iLine;
                iCell = nCells;
            }
        }
        return (iLine + 1) * lineHeight + iLine * spacing() + 2 * margin() + 10;
    }
    virtual QSize sizeHint() const override {
        return minimumSize();
    }
    virtual void addItem(QLayoutItem* item) override {
        _items.append(Item(item));
    }
    virtual int count() const override { return _items.count(); }
    virtual QLayoutItem* itemAt(int index) const override {
        return _items.value(index).layoutItem;
    }
    virtual QLayoutItem* takeAt(int index) override {
        if (index >= 0 && index < _items.size())
            return _items.takeAt(index).layoutItem;
        else
            return nullptr;
    }
    virtual void setGeometry(const QRect& r) override {
        QLayout::setGeometry(r);
        fluidLayout(r);
    }

public:
    void addWidget(QWidget* w, Item::Size size) {
        addChildWidget(w);
        QWidgetItem* widgetItem = new QWidgetItemV2(w);
        _items.append(Item(widgetItem, size));
        invalidate();
    }

private:
    void fluidLayout(const QRect& r) {
        const QRect effectiveRect = getEffectiveRect(r);
        int iLine = 0, iCell = 0;
        for (Item itemHolder : _items) {
            QLayoutItem* item = itemHolder.layoutItem;
            int nCells =
                    getNumberOfCells(effectiveRect.size(), itemHolder.size);
            if (iCell + nCells <= nCellsPerLine) {
                item->setGeometry(getRect(effectiveRect, iLine, iCell, nCells));
                iCell += nCells;
            } else {
                item->setGeometry(getRect(effectiveRect, iLine + 1, 0, nCells));
                ++iLine;
                iCell = nCells;
            }
        }
    }
    int getNumberOfCells(
            const QSize& effectiveRectSize, Item::Size size) const {
        if (Item::FullLine == size) {
            return nCellsPerLine;
        }
        if (Item::Minimum == size) {
            return minimumCellCount;
        }
        return getNumberOfCells(
                effectiveRectSize.width(), getMinimumWidth(size));
    }
    int getNumberOfCells(int lineWidth, int minWidth) const {
        const auto& arrayOfCellCounts = possibleCellCounts();
        for (auto nCells : arrayOfCellCounts) {
            if (minWidth < getWidthOfCells(lineWidth, nCells)) {
                return nCells;
            }
        }
        return nCellsPerLine;
    }
    int getMinimumWidth(Item::Size size) const {
        static QMap<Item::Size, int> sizeToWidth{
            {Item::Large, 30 * QFontMetrics(QFont()).averageCharWidth()},
            {Item::Medium, 15 * QFontMetrics(QFont()).averageCharWidth()},
            {Item::Small, 10 * QFontMetrics(QFont()).averageCharWidth()}
        };
        auto width = sizeToWidth.value(size, -1);
        assert(width >= 0);
        return width;
    }
    const std::vector<int>& possibleCellCounts() const {
        static std::vector<int> nCells;
        if (nCells.empty()) {
            int sqrt = qSqrt(nCellsPerLine);
            nCells.reserve(2 * sqrt);
            for (int num = 1; num <= sqrt; ++num) {
                if (nCellsPerLine % num == 0) {
                    nCells.push_back(num);
                    nCells.push_back(nCellsPerLine / num);
                }
            }
            std::sort(nCells.begin(), nCells.end());
        }
        return nCells;
    }
    int getWidthOfCells(int lineWidth, int nCells) const {
        float lineWidthWithoutSpacing =
                lineWidth - (nCellsPerLine - 1) * spacing();
        float singleCellWidth = lineWidthWithoutSpacing / nCellsPerLine;
        return nCells * singleCellWidth + (nCells - 1) * spacing();
    }
    QRect getRect(const QRect& effectiveRect, int iLine, int iCell,
            int nCells) const {
        float lineWidthWithoutSpacing =
                effectiveRect.width() - (nCellsPerLine - 1) * spacing();
        float singleCellWidth = lineWidthWithoutSpacing / nCellsPerLine;
        int x = effectiveRect.x() + iCell * singleCellWidth + iCell * spacing();
        int y = effectiveRect.y() + iLine * lineHeight + iLine * spacing();
        int w = getWidthOfCells(effectiveRect.width(), nCells);
        int h = lineHeight;
        auto rect = QRect(x, y, w, h);
        return rect;
    }
    QRect getEffectiveRect(const QRect& r) const {
        return r.adjusted(+margin(), +margin(), -margin(), -margin());
    }

private:
    const int lineHeight = 40;
    const int nCellsPerLine = 12;
    const int minimumCellCount = 1;
    QList<Item> _items;
};

/**
 * @brief The LabeledWidget class
 */
template <typename T>
class LabeledWidget : public QWidget {
public:
    LabeledWidget(QString text = tr(""), QWidget* parent = nullptr)
          : QWidget(parent) {
        auto setWidgetWindowTextColor =
                [](QWidget* widget, const QColor& color) {
            auto palette = widget->palette();
            palette.setColor(QPalette::WindowText, color);
            palette.setColor(QPalette::ButtonText, color);
            palette.setColor(QPalette::Text, color);
            widget->setPalette(palette);
        };
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(5);
        _label = new QLabel(text, this);
        setWidgetWindowTextColor(_label, QColor(40, 40, 40));
        _label->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
        layout->addWidget(_label);
        _widget = new T(this);
        setWidgetWindowTextColor(_widget, QColor(80, 80, 80));
        layout->addWidget(_widget);
    }

public:
    T* widget() const { return _widget; }
    void setLabel(const QString& text) { _label->setText(text); }

private:
    T* _widget = nullptr;
    QLabel* _label = nullptr;
};

/**
 * @brief The LazyPanel class
 */
class LazyPanel : public QWidget {
    Q_OBJECT
public:
    LazyPanel(QWidget* parent = nullptr) : QWidget(parent) {
        _layout = new FluidLayout(this);
        _layout->setMargin(0);
        _layout->setSpacing(5);
    }

public:
    void addWidget(QWidget* widget, FluidLayout::Item::Size size) {
        _layout->addWidget(widget, size);
    }

private:
    FluidLayout* _layout = nullptr;
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
        button(name, FluidLayout::Item::Medium, context, func);
    }
    void button(const QString& name, FluidLayout::Item::Size size,
            QObject* context, std::function<void()> func) {
        if (!_widgets.contains(name)) {
            auto button = new QPushButton(name);
            _panel->addWidget(button, size);
            _widgets[name] = button;
        }
        QPushButton* button = static_cast<QPushButton*>(_widgets[name]);
        disconnect(button, nullptr, context, nullptr);
        connect(button, &QPushButton::clicked, context, func);
    }
    void labeledButton(const QString& label, const QString& buttonText,
            QObject* context, std::function<void()> func) {
        labeledButton(
                label, buttonText, context, FluidLayout::Item::Medium, func);
    }
    void labeledButton(const QString& label, const QString& buttonText,
            QObject* context, FluidLayout::Item::Size size,
            std::function<void()> func) {
        if (!_widgets.contains(buttonText)) {
            auto labeledButton = new LabeledWidget<QPushButton>(label);
            labeledButton->widget()->setText(buttonText);
            _panel->addWidget(labeledButton, size);
            _widgets[buttonText] = labeledButton;
        }
        auto labeledButton =
                static_cast<LabeledWidget<QPushButton>*>(_widgets[buttonText]);
        auto button = labeledButton->widget();
        disconnect(button, nullptr, context, nullptr);
        connect(button, &QPushButton::clicked, context, func);
    }

    QString labeledCombo(const QString& key) {
        auto combo =
                static_cast<LabeledWidget<QComboBox>*>(_widgets[key])->widget();
        return combo->currentText();
    }
    void labeledCombo(const QString& key, const QString& option) {
        auto combo =
                static_cast<LabeledWidget<QComboBox>*>(_widgets[key])->widget();
        combo->blockSignals(true);
        combo->setCurrentText(option);
        combo->blockSignals(false);
    }
    void labeledCombo(
            QString key, const QString& label, FluidLayout::Item::Size size) {
        if (!_widgets.contains(key)) {
            auto labeledCombo = new LabeledWidget<QComboBox>(label);
            _panel->addWidget(labeledCombo, size);
            _widgets[key] = labeledCombo;
        }
    }
    void labeledCombo(QString key, const QString& label,
            const QStringList& items, QObject* context,
            std::function<void(const QString&)> func) {
        labeledCombo(
                key, label, items, FluidLayout::Item::Medium, context, func);
    }
    void labeledCombo(QString key, const QString& label,
            const QStringList& items, FluidLayout::Item::Size size,
            QObject* context, std::function<void(const QString&)> func) {
        try {
            labeledCombo(key, label, items, items.at(0), size, context, func);
        } catch (...) {
            std::cout << "LazyUI::labeledCombo" << std::endl;
        }
    }
    void labeledCombo(
            QString key, const QStringList& items, QObject* context,
            std::function<void(const QString&)> func) {
        try {
            labeledCombo(key, items, items.at(0), context, func);
        } catch (...) {
            std::cout << "LazyUI::labeledCombo" << std::endl;
        }
    }
    void labeledCombo(
            QString key, const QStringList& items, const QString& select,
            QObject* context, std::function<void(const QString&)> func) {
        auto combo =
                static_cast<LabeledWidget<QComboBox>*>(_widgets[key])->widget();
        combo->blockSignals(true);
        combo->clear();
        combo->addItems(items);
        combo->setCurrentText(select);
        combo->blockSignals(false);
        disconnect(combo, nullptr, context, nullptr);
        connect(combo, &QComboBox::currentTextChanged, context, func);
    }
    void labeledCombo(QString key, const QString& label,
            const QStringList& items, const QString& select,
            FluidLayout::Item::Size size, QObject* context,
            std::function<void(const QString&)> func) {
        labeledCombo(key, label, size);
        labeledCombo(key, items, select, context, func);
    }

    void labeledScrollBar(QString key, const QString& label) {
        assert(_widgets.contains(key));
        auto labeledScrollBar =
                static_cast<LabeledWidget<QScrollBar>*>(_widgets[key]);
        labeledScrollBar->setLabel(label);
    }
    void labeledScrollBar(QString key, const QString& label, int value) {
        assert(_widgets.contains(key));
        auto labeledScrollBar =
                static_cast<LabeledWidget<QScrollBar>*>(_widgets[key]);
        labeledScrollBar->setLabel(label);
        auto scrollBar = labeledScrollBar->widget();
        scrollBar->blockSignals(true);
        scrollBar->setValue(value);
        scrollBar->blockSignals(false);
    }
    void labeledScrollBar(
            QString key, const QString& label, FluidLayout::Item::Size size) {
        if (!_widgets.contains(key)) {
            auto labeledScrollBar = new LabeledWidget<QScrollBar>(label);
            labeledScrollBar->widget()->setOrientation(Qt::Horizontal);
            _panel->addWidget(labeledScrollBar, size);
            _widgets[key] = labeledScrollBar;
        }
    }
    void labeledScrollBar(QString key, const QString& label, int minimum,
            int maximum, int value, QObject* context,
            std::function<void(int)> func) {
        labeledScrollBar(key, label, minimum, maximum, value,
                FluidLayout::Item::Medium, context, func);
    }
    void labeledScrollBar(QString key, const QString& label, int minimum,
            int maximum, int value, FluidLayout::Item::Size size,
            QObject* context, std::function<void(int)> func) {
        labeledScrollBar(key, label, size);
        auto labeledScrollBar =
                static_cast<LabeledWidget<QScrollBar>*>(_widgets[key]);
        labeledScrollBar->setLabel(label);
        auto scrollBar = labeledScrollBar->widget();
        scrollBar->blockSignals(true);
        scrollBar->setPageStep(1);
        scrollBar->setRange(minimum, maximum);
        scrollBar->setValue(value);
        scrollBar->blockSignals(false);
        disconnect(scrollBar, nullptr, context, nullptr);
        connect(scrollBar, &QScrollBar::valueChanged, context, func);
    }

    void labeledLineEdit(QString key, const QString& label) {
        auto labeledLineEdit =
                static_cast<LabeledWidget<QLineEdit>*>(_widgets[key]);
        labeledLineEdit->setLabel(label);
    }
    void labeledLineEdit(
            QString key, const QString& label, const QString& text) {
        auto labeledLineEdit =
                static_cast<LabeledWidget<QLineEdit>*>(_widgets[key]);
        labeledLineEdit->setLabel(label);
        QLineEdit* lineEdit = labeledLineEdit->widget();
        lineEdit->blockSignals(true);
        lineEdit->setText(text);
        lineEdit->blockSignals(false);
    }
    void labeledLineEdit(QString key, const QString& label, const QString& text,
            FluidLayout::Item::Size size, QObject* context,
            std::function<void(const QString& text)> func) {
        if (!_widgets.contains(key)) {
            auto labeledLineEdit = new LabeledWidget<QLineEdit>(label);
            _panel->addWidget(labeledLineEdit, size);
            _widgets[key] = labeledLineEdit;
        }
        labeledLineEdit(key, label, text);
        auto labeledLineEdit =
                static_cast<LabeledWidget<QLineEdit>*>(_widgets[key]);
        QLineEdit* lineEdit = labeledLineEdit->widget();
        disconnect(lineEdit, nullptr, context, nullptr);
        connect(lineEdit, &QLineEdit::textEdited, context, func);
    }

    void labeledDivider(QString key, const QString& text) {
        if (!_widgets.contains(key)) {
            auto header = new LabeledWidget<QFrame>(text);
            QFrame* divider = header->widget();
            divider->setFrameShape(QFrame::HLine);
            divider->setLineWidth(2);
            divider->setFrameShadow(QFrame::Plain);
            _panel->addWidget(header, FluidLayout::Item::FullLine);
            _widgets[key] = header;
        }
    }
    void divider(QString key) {
        if (!_widgets.contains(key)) {
            QFrame* divider = new QFrame();
            divider->setFrameShape(QFrame::HLine);
            divider->setLineWidth(2);
            divider->setFrameShadow(QFrame::Plain);
            _panel->addWidget(divider, FluidLayout::Item::FullLine);
            _widgets[key] = divider;
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
