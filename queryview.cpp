#include "queryview.h"
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDebug>

std::ostream &operator<<(std::ostream &os, const QueryRule &rule)
{
    os << "Histogram Name: " << rule.histName << std::endl;
    for (auto interval : rule.intervals) {
        os << "Interval: (" << interval.lower << ", " << interval.upper << ")"
                << std::endl;
    }
    os << "Threshold: " << rule.threshold << std::endl;
    return os;
}

bool operator==(const QueryRule &a, const QueryRule &b)
{
    if (a.histName != b.histName)
        return false;
    if (fabs(a.threshold - b.threshold) > 0.0001)
        return false;
    if (a.intervals != b.intervals)
        return false;
    return true;
}

/**
 * @brief QueryRuleDialog::QueryRuleDialog
 * @param parent
 */
QueryRuleDialog::QueryRuleDialog(QWidget *parent) : QDialog(parent)
{
    histComboLabel = new QLabel("histogram: ");
    histCombo = new QComboBox();
    connect(histCombo,
            static_cast<void(QComboBox::*)(int)>(
                &QComboBox::currentIndexChanged),
            this, &QueryRuleDialog::selectHistConfig);

    thresholdLabel      = new QLabel( "Threshold ( >= ) : " );
    thresholdLabelUnits = new QLabel( "%" );
    thresholdLineEdit   = new QLineEdit;
    thresholdLineEdit->setText( "0" );

    label1 = new QLabel(tr("Attribute 1:"));
    combo1 = new QComboBox;
    label1->setBuddy(combo1);

    label2 = new QLabel(tr(" to "));
    combo2 = new QComboBox;
    label2->setBuddy(combo2);

    label3 = new QLabel(tr("Attribute 2:"));
    combo3 = new QComboBox;
    label3->setBuddy(combo3);

    label4 = new QLabel(tr(" to "));
    combo4 = new QComboBox;
    label4->setBuddy(combo4);

    label5 = new QLabel(tr("Attribute 2:"));
    combo5 = new QComboBox;
    label5->setBuddy(combo5);

    label6 = new QLabel(tr(" to "));
    combo6 = new QComboBox;
    label6->setBuddy(combo6);

    selectButton = new QPushButton(tr("&Select"));
    selectButton->setDefault(true);
    selectButton->setEnabled(true);

    label1->setMinimumWidth( 90 );
    label2->setMinimumWidth( 10 );
    label3->setMinimumWidth( 90 );
    label4->setMinimumWidth( 10 );
    label5->setMinimumWidth( 90 );
    label6->setMinimumWidth( 10 );

    combo1->setMinimumWidth( 100 );
    combo2->setMinimumWidth( 100 );
    combo3->setMinimumWidth( 100 );
    combo4->setMinimumWidth( 100 );
    combo5->setMinimumWidth( 100 );
    combo6->setMinimumWidth( 100 );

    thresholdLabel->setMinimumWidth(90);

    closeButton = new QPushButton(tr("Close"));

    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));

    connect(selectButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout *histSelectLayout = new QHBoxLayout();
    histSelectLayout->addWidget( histComboLabel );
    histSelectLayout->addWidget( histCombo );
    histSelectLayout->addStretch();

    tWidget = new QWidget;
    QHBoxLayout *thresholdLayout = new QHBoxLayout;
    thresholdLayout->addWidget( thresholdLabel      );
    thresholdLayout->addWidget( thresholdLineEdit   );
    thresholdLayout->addWidget( thresholdLabelUnits );
    tWidget->setLayout( thresholdLayout );

    r1Widget = new QWidget;
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(label1);
    topLayout->addWidget(combo1);
    topLayout->addWidget(label2);
    topLayout->addWidget(combo2);
    r1Widget->setLayout( topLayout );

    r2Widget = new QWidget;
    QHBoxLayout *middleLayout = new QHBoxLayout;
    middleLayout->addWidget(label3);
    middleLayout->addWidget(combo3);
    middleLayout->addWidget(label4);
    middleLayout->addWidget(combo4);
    r2Widget->setLayout( middleLayout );

    r3Widget = new QWidget;
    QHBoxLayout *middleLayout2 = new QHBoxLayout;
    middleLayout2->addWidget(label5);
    middleLayout2->addWidget(combo5);
    middleLayout2->addWidget(label6);
    middleLayout2->addWidget(combo6);
    r3Widget->setLayout( middleLayout2 );

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(selectButton);
    bottomLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    mainLayout->addLayout(histSelectLayout);
    mainLayout->addWidget( r1Widget );
    mainLayout->addWidget( r2Widget );
    mainLayout->addWidget( r3Widget );
    mainLayout->addWidget( tWidget  );
    mainLayout->addLayout(bottomLayout);
    setLayout(mainLayout);

    r1Widget->hide();
    r2Widget->hide();
    r3Widget->hide();
}

QueryRule QueryRuleDialog::getRule(
        std::vector<HistConfig> histConfigs, QWidget *parent) {
    QueryRuleDialog dialog(parent);
    dialog.setHistConfigs(histConfigs);
    if (QDialog::Rejected == dialog.exec())
        return QueryRule();
    return dialog.getRule();
}

void QueryRuleDialog::setHistConfigs(std::vector<HistConfig> histConfigs)
{
    _histConfigs = histConfigs;
    histCombo->blockSignals(true);
    histCombo->clear();
    for (unsigned int iConfig = 0; iConfig < histConfigs.size(); ++iConfig) {
        const HistConfig& config = histConfigs[iConfig];
        histCombo->addItem(QString::fromStdString(config.name()));
    }
    histCombo->setCurrentIndex(0);
    histCombo->blockSignals(false);
    selectHistConfig(0);
}

QueryRule QueryRuleDialog::getRule() const
{
    QueryRule rule;
    const HistConfig& config = _histConfigs[histCombo->currentIndex()];
    rule.histName = config.name();
    rule.intervals.resize(config.nDim);
    if (config.nDim > 0) {
        rule.intervals[0].lower = combo1->currentText().toFloat();
        rule.intervals[0].upper = combo2->currentText().toFloat();
    }
    if (config.nDim > 1) {
        rule.intervals[1].lower = combo3->currentText().toFloat();
        rule.intervals[1].upper = combo4->currentText().toFloat();
    }
    if (config.nDim > 2) {
        rule.intervals[2].lower = combo5->currentText().toFloat();
        rule.intervals[2].upper = combo6->currentText().toFloat();
    }
    rule.threshold = thresholdLineEdit->text().toFloat() / 100.f;
    return rule;
}

void QueryRuleDialog::selectHistConfig(int iConfig)
{
    const HistConfig& config = _histConfigs[iConfig];
    thresholdLineEdit->setText("0");
    auto populateRow =
            [](QLabel* label, QComboBox* combo1, QComboBox* combo2,
                const HistConfig& config, int index) {
        label->setText(QString::fromStdString(config.vars[index]));
        combo1->blockSignals(true);
        combo2->blockSignals(true);
        combo1->clear();
        combo2->clear();
        /// TODO: let users input arbitrary number instead of limited to 10
        /// possible values.
        for (int i = 0; i <= 10; ++i) {
            float ratio = float(i) / 10.f;
            /// TODO: use real value from the data instead of percentage. It
            /// requires iterating over all the histograms to calculate the mins
            /// and maxs.
            float inter = ratio;
//            float inter = config.mins[index] * (1.f - ratio)
//                    + config.maxs[index] * ratio;
            combo1->addItem(QString::number(inter));
            combo2->addItem(QString::number(inter));
        }
        combo1->blockSignals(false);
        combo2->blockSignals(false);
        combo1->setCurrentIndex(0);
        combo2->setCurrentIndex(combo2->count() - 1);
    };
    r1Widget->hide();
    r2Widget->hide();
    r3Widget->hide();
    if (config.nDim > 0) {
        populateRow(label1, combo1, combo2, config, 0);
        r1Widget->show();
    }
    if (config.nDim > 1) {
        populateRow(label3, combo3, combo4, config, 1);
        r2Widget->show();
    }
    if (config.nDim > 2) {
        populateRow(label5, combo5, combo6, config, 2);
        r3Widget->show();
    }
}

/**
 * @brief QueryRuleView::QueryRuleView
 * @param parent
 */
QueryRuleView::QueryRuleView(QWidget *parent)
  : QFrame(parent)
  , _label(new QLabel("rule text", this))
  , _crossBtn(new QPushButton("X", this))
{
    construct();
}

QueryRuleView::QueryRuleView(QueryRule rule, QWidget *parent)
  : QFrame(parent)
  , _label(new QLabel(this))
  , _crossBtn(new QPushButton("X", this))
  , _rule(rule)
{
    construct();
    QString intervalsText;
    intervalsText += QString("(%1,%2)")
            .arg(rule.intervals[0].lower).arg(rule.intervals[0].upper);
    for (unsigned int i = 1; i < rule.intervals.size(); ++i) {
        intervalsText +=
                QString(",(%1,%2)")
                    .arg(rule.intervals[i].lower)
                    .arg(rule.intervals[i].upper);
    }
    QString text = QString::fromStdString(rule.histName)
            + QString(" with > %1% of frequency within intervals (")
                .arg(rule.threshold * 100.f)
            + intervalsText + ")";
    _label->setText(text);
}

void QueryRuleView::enterEvent(QEvent*)
{
    _crossBtn->setVisible(true);
}

void QueryRuleView::leaveEvent(QEvent*)
{
    _crossBtn->setVisible(false);
}

void QueryRuleView::construct()
{
    this->setFixedHeight(30);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    layout->addWidget(_label, 1);
    layout->addWidget(_crossBtn, 0);
    _crossBtn->setVisible(false);
    _crossBtn->setFixedSize(19, 19);
    connect(_crossBtn, &QPushButton::clicked,
            this, &QueryRuleView::emitRemoveMe);
}

/**
 * @brief QueryView::QueryView
 * @param parent
 */
QueryView::QueryView(QWidget *parent)
  : Widget(parent, Qt::Tool)
  , _layout(new QVBoxLayout())
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget([this]() {
        QPushButton* addFilterBtn = new QPushButton("Add Filter", this);
        connect(addFilterBtn, &QPushButton::clicked,
                this, &QueryView::addFilter);
        return addFilterBtn;
    }());
    layout->addLayout(_layout);
}

void QueryView::setHistConfigs(std::vector<HistConfig> histConfigs)
{
    _histConfigs = histConfigs;
}

void QueryView::addFilter()
{
    QueryRule rule = QueryRuleDialog::getRule(_histConfigs, this);
    if (rule.isEmpty())
        return;
    _rules.push_back(rule);
    rulesUpdated();
}

void QueryView::removeRule(QueryRuleView *ruleView)
{
    auto itr = std::find(_rules.begin(), _rules.end(), ruleView->rule());
    if (itr != _rules.end()) {
        _rules.erase(itr);
    }
    rulesUpdated();
}

void QueryView::rulesUpdated()
{
    emit rulesChanged(_rules);
    updateRuleViews();
}

void QueryView::updateRuleViews()
{
    QLayoutItem* child;
    while ((child = _layout->takeAt(0)) != 0) {
        if (child->widget() != NULL)
            delete (child->widget());
        delete child;
    }
    for (auto rule : _rules) {
        QueryRuleView* ruleView = new QueryRuleView(rule, this);
        _layout->addWidget(ruleView);
        connect(ruleView, &QueryRuleView::removeMe,
                this, &QueryView::removeRule);
    }
}
