#ifndef QUERYVIEW_H
#define QUERYVIEW_H

#include <widget.h>
#include <QFrame>
#include <QDialog>
#include <data/DataPool.h>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QComboBox;
class QLineEdit;

class QueryRule {
public:
    bool isEmpty() const { return histName.empty(); }

public:
    std::string histName;
    std::vector<Interval<float>> intervals;
    float threshold;
};
std::ostream& operator<<(std::ostream& os, const QueryRule& rule);
bool operator==(const QueryRule& a, const QueryRule& b);

/**
 * @brief The QueryRuleDialog class
 */
class QueryRuleDialog : public QDialog {
    Q_OBJECT
public:
    explicit QueryRuleDialog(QWidget* parent = 0);
    static QueryRule getRule(
            std::vector<HistConfig> histConfigs, QWidget* parent = 0);

public:
    void setHistConfigs(std::vector<HistConfig> histConfigs);
    QueryRule getRule() const;

private:
    void selectHistConfig(int iConfig);

private:
    std::vector<HistConfig> _histConfigs;

private:
    QLabel * histComboLabel;
    QComboBox * histCombo;

    QLabel * thresholdLabel;
    QLabel * thresholdLabelUnits;
    QLineEdit * thresholdLineEdit;

    QLabel *label1;
    QComboBox *combo1;
    QLabel *label2;
    QComboBox *combo2;

    QLabel *label3;
    QComboBox *combo3;
    QLabel *label4;
    QComboBox *combo4;

    QLabel *label5;
    QComboBox *combo5;
    QLabel *label6;
    QComboBox *combo6;

    QWidget * r1Widget;
    QWidget * r2Widget;
    QWidget * r3Widget;
    QWidget * tWidget;

    QPushButton *selectButton;
    QPushButton *closeButton;
};

/**
 * @brief The QueryRuleView class
 */
class QueryRuleView : public QFrame {
    Q_OBJECT
public:
    explicit QueryRuleView(QWidget *parent = 0);
    explicit QueryRuleView(QueryRule rule, QWidget* parent = 0);
    ~QueryRuleView() {}

signals:
    void removeMe(QueryRuleView*);

public:
    void emitRemoveMe() { emit removeMe(this); }
    const QueryRule& rule() const { return _rule; }

protected:
    virtual void enterEvent(QEvent*) override;
    virtual void leaveEvent(QEvent*) override;

private:
    void construct();

private:
    QLabel* _label;
    QPushButton* _crossBtn;
    QueryRule _rule;
};

/**
 * @brief The QueryView class
 */
class QueryView : public Widget
{
    Q_OBJECT
public:
    explicit QueryView(QWidget *parent = 0);

signals:
    void rulesChanged(const std::vector<QueryRule>&);

public:
    void setHistConfigs(std::vector<HistConfig> histConfigs);
    void addFilter();
    void removeRule(QueryRuleView* ruleView);

private:
    void rulesUpdated();
    void updateRuleViews();

private:
    QVBoxLayout* _layout;
    std::vector<HistConfig> _histConfigs;
    /// TODO: turn this into a map?
    std::vector<QueryRule> _rules;
};

#endif // QUERYVIEW_H
