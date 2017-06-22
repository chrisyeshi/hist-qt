#ifndef HISTDIMSCOMBO_H
#define HISTDIMSCOMBO_H

#include <widget.h>
#include <QComboBox>
#include <QHBoxLayout>

class HistDimsCombo : public Widget {
    Q_OBJECT
public:
    explicit HistDimsCombo(QWidget* parent) : Widget(parent) {
        _combo = new QComboBox(this);
        connect(_combo, &QComboBox::currentTextChanged,
                this, [this](QString itemStr) {
            std::vector<int> dims = _varsToDims[itemStr];
            emit dimsChanged(dims);
            emit varsChanged(dimsToVars(dims));
        });
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->addWidget(_combo);
        layout->setMargin(0);
        layout->setSpacing(0);
    }

public:
    void setItems(const std::vector<std::string>& vars) {
        _vars = vars;
        _combo->clear();
        _varsToDims.clear();
        for (int i = 0; i < int(vars.size()); ++i)
        for (int j = i + 1; j < int(vars.size()); ++j) {
            QString str = QString::fromStdString(vars[i] + "-" + vars[j]);
            _varsToDims.insert(str, {i, j});
            _combo->addItem(str);
        }
        for (int i = 0; i < int(vars.size()); ++i) {
            QString str = QString::fromStdString(vars[i]);
            _varsToDims.insert(str, {i});
            _combo->addItem(str);
        }
    }

    void setCurrentIndex(int index) {
        _combo->setCurrentIndex(index);
    }

    void setCurrentDims(const std::vector<int>& dims) {
        setCurrentVars(dimsToVars(dims));
    }

    std::vector<int> currentDims() const {
        return _varsToDims[_combo->currentText()];
    }

    void setCurrentVars(const std::vector<std::string>& vars) {
        std::string str = vars[0];
        for (unsigned int i = 1; i < vars.size(); ++i) {
            str += '-' + vars[i];
        }
        _combo->setCurrentText(QString::fromStdString(str));
    }

    std::vector<std::string> currentVars() const {
        return dimsToVars(currentDims());
    }

signals:
    void dimsChanged(std::vector<int> dims);
    void varsChanged(std::vector<std::string> vars);

private:
    std::vector<std::string> dimsToVars(const std::vector<int>& dims) const {
        std::vector<std::string> vars(dims.size());
        for (unsigned int i = 0; i < dims.size(); ++i) {
            vars[i] = _vars[dims[i]];
        }
        return vars;
    }

private:
    QComboBox* _combo;
    QMap<QString, std::vector<int>> _varsToDims;
    std::vector<std::string> _vars;
};

#endif // HISTDIMSCOMBO_H
