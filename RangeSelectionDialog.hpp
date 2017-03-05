#ifndef RANGESELECTIONDIALOG
#define RANGESELECTIONDIALOG

#include <string>
#include <vector>
#include <unordered_map>

#include <QtMath>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QDebug>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>

#include <data/DataPool.h>
#include <glm/vec2.hpp>

class RangeSelectionDialog : public QDialog
{
    Q_OBJECT

public:

signals:

    void poloidalSectionSelected( std::pair< glm::vec2, glm::vec2 > selection, std::pair< std::string, std::string > attributes );

private slots:

    void selectHist( int index )
    {
        r1Widget->show();
        label1->setText( histAttributes[ index ][ 0 ].c_str() );
        combo1->clear();
        combo2->clear();
        float dv = ( histRanges[ index ][ 0 ].b() - histRanges[ index ][ 0 ].a() ) / ( double ) histBinWidths[ index ][ 0 ];
        for( int i = 0; i <= histBinWidths[ index ][ 0 ] ; ++i )
        {
            combo1->addItem( QString::number( histRanges[ index ][ 0 ].a() + dv*i ) );
            combo2->addItem( QString::number( histRanges[ index ][ 0 ].a() + dv*i ) );
        }
        combo2->setCurrentIndex(combo2->count() - 1);

        if( histBinWidths[ index ].size() > 1 )
        {
            r2Widget->show();
            label3->setText( histAttributes[ index ][ 1 ].c_str() );
            combo3->clear();
            combo4->clear();
            float dv = ( histRanges[ index ][ 1 ].b() - histRanges[ index ][ 1 ].a() ) / ( double ) histBinWidths[ index ][ 1 ];
            for( int i = 0; i <= histBinWidths[ index ][ 1 ] ; ++i )
            {
                combo3->addItem( QString::number( histRanges[ index ][ 1 ].a() + dv*i ) );
                combo4->addItem( QString::number( histRanges[ index ][ 1 ].a() + dv*i ) );
            }
            combo4->setCurrentIndex(combo4->count() - 1);
        }
        else
        {
            r2Widget->hide();
        }

        if( histBinWidths[ index ].size() > 2 )
        {
            r3Widget->show();
            label5->setText( histAttributes[ index ][ 2 ].c_str() );
            combo5->clear();
            combo6->clear();
            float dv = ( histRanges[ index ][ 2 ].b() - histRanges[ index ][ 2 ].a() ) / ( double ) histBinWidths[ index ][ 2 ];
            for( int i = 0; i <= histBinWidths[ index ][ 2 ] ; ++i )
            {
                combo5->addItem( QString::number( histRanges[ index ][ 2 ].a() + dv*i ) );
                combo6->addItem( QString::number( histRanges[ index ][ 2 ].a() + dv*i ) );
            }
            combo6->setCurrentIndex(combo6->count() - 1);
        }
        else
        {
            r3Widget->hide();
        }

        tWidget->show();
    }

private:

    std::vector< std::string >  histKeys;
    std::vector< std::vector< glm::vec2 > > histRanges;
    std::vector< std::vector< std::int32_t > > histBinWidths;
    std::vector< std::vector< std::string > >  histAttributes;

    QLabel * histComboLabel;
    QComboBox * histCombo;

    QLabel * thresholdLabel;
    QLabel * thresholdLabelUnits;
    QLineEdit * thresholdLineEdit;

    std::unordered_map< std::string, glm::vec2 > rangeMap;
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

signals:

    void rangeFilterSelected(
        std::string histKey,
        std::vector< std::pair< int32_t, int32_t > > binRanges,
        float threshold );

private slots:

    void select()
    {
        int index = histCombo->currentIndex();
        std::vector< std::pair< int32_t, int32_t > > binRanges;
        binRanges.push_back( std::pair< int32_t, int32_t >( combo1->currentIndex(), combo2->currentIndex() - 1 ) );
        if( histBinWidths[ index ].size() > 1  )
        {
            binRanges.push_back( std::pair< int32_t, int32_t >( combo3->currentIndex(), combo4->currentIndex() - 1 ) );
        }
        if( histBinWidths[ index ].size() > 2  )
        {
            binRanges.push_back( std::pair< int32_t, int32_t >( combo5->currentIndex(), combo6->currentIndex() - 1 ) );
        }
        emit rangeFilterSelected( histCombo->currentText().toStdString(), binRanges, std::stof( thresholdLineEdit->text().toStdString() ) );
        close();
    }

public:

    void setHistograms(
        const std::vector< std::string > & _histKeys,
        const std::vector< std::vector< glm::vec2 > > & _histRanges,
        const std::vector< std::vector< std::int32_t > > & _histBinWidths,
        const std::vector< std::vector< std::string > > & _histAttributes  )
    {
        histKeys       = _histKeys;
        histRanges     = _histRanges;
        histBinWidths  = _histBinWidths;
        histAttributes = _histAttributes;

        for( auto k : histKeys )
        {
            histCombo->addItem( k.c_str() );
        }

        selectHist( 0 );
    }

    void setHistograms(const std::vector<HistConfig>& histConfigs) {
        std::vector<std::string> keys(histConfigs.size());
        std::vector<std::vector<glm::vec2> ranges(histConfigs.size());
        std::vector<std::vector<int>> binWidths;
        std::vector<std::vector<std::string>> attributes;
        for (unsigned int iConfig = 0; iConfig < histConfigs; ++iConfig) {
            const HistConfig& config = histConfigs[iConfig];
            keys[iConfig] = config.name();
            ranges[iConfig].resize(config.nDim);
            for (int iDim = 0; iDim < config.nDim; ++iDim) {
                ranges[iConfig][iDim] =
                        glm::vec2(config.mins[iDim], config.maxs[iDim]);
            }
        }
    }

    RangeSelectionDialog( QWidget *parent = 0 ) : QDialog( parent )
    {
//        setStyleSheet("background-color: rgb( 240, 240, 240 );");

        histComboLabel = new QLabel( "histogram: " );
        histCombo = new QComboBox();
        connect( histCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(selectHist(int)) );

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

        connect(closeButton, SIGNAL(clicked()),
                this, SLOT(close()));

        connect(selectButton, SIGNAL(clicked()),
                this, SLOT(select()));

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
        tWidget->hide();
    }
};

#endif // RANGESELECTIONDIALOG
