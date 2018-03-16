#include "userstudyctrlview.h"
#include "ui_userstudyctrlview.h"

UserStudyCtrlView::UserStudyCtrlView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserStudyCtrlView) {
    ui->setupUi(this);
    QImage image("../interesting_histogram.png");
//    auto scaled = image.scaledToHeight(250, Qt::SmoothTransformation);
    ui->image->setScaledContents(true);
    ui->image->setPixmap(QPixmap::fromImage(image));
    connect(ui->next, &QPushButton::clicked,
            this, &UserStudyCtrlView::nextButtonClicked);
}

UserStudyCtrlView::~UserStudyCtrlView() {
    delete ui;
}

void UserStudyCtrlView::setCurrentPage(int pageId) {
    _pageId = pageId;
    if (0 == _pageId) {
        ui->title->setText("Tutorial");
        ui->content->setPlainText(
                tr("Walk-through all the controls and explain the ") +
                tr("functionalities, including histogram configs, ") +
                tr("orientations, frequency range, variable ranges ") +
                tr("(brushing), pan and zoom, histogram selection, detailed ") +
                tr("view, histogram merging, and time steps."));
        ui->image->hide();
    } else if (2 == _pageId) {
        ui->title->setText("Exercise");
        ui->content->setPlainText(
                tr("Change the visible range of the vertical axis to") +
                tr(" hide frequency values that are below 20%."));
        ui->image->hide();
    } else if (4 == _pageId) {
        ui->title->setText("Exercise");
        ui->content->setPlainText(
                tr("Change into 2D histograms. What is the variable ranges ") +
                tr("of the current slice?"));
        ui->image->hide();
    } else if (6 == _pageId) {
        ui->title->setText("Exercise");
        ui->content->setPlainText(
                tr("Select an interesting 2D histogram at ") +
                tr("time step 8.5210E-04 and change the varialbe ranges to ") +
                tr("the most interesting portion of that histogram."));
        ui->image->hide();
    } else if (8 == _pageId) {
        ui->title->setText("User Study");
        ui->content->setPlainText(
                tr("The particular heat release range of interest is from ") +
                "-2e+10 to -1e+10. The samples in this range represents a " +
                "flame is burning in the particular region. Describe what is " +
                "happening within this range spatially and temporally.");
        ui->image->hide();
    } else if (10 == _pageId) {
        ui->title->setText("User Study");
        ui->content->setPlainText(
                tr("Select histograms that represent regions of quenching ") +
                tr("in time step 6.3110E-04. Quenching typically occurs in ") +
                tr("the near-wall regions. In this case, the quenching ") +
                tr("events can be identified with significant ") +
                tr("presence of CH2O and absence of heat release. "));
        ui->image->hide();
    } else if (12 == _pageId) {
        ui->title->setText("User Study");
        ui->content->setPlainText(
                tr("Select subdomains where flame propagation is likely ") +
                tr("to be present. This task uses the data quantified by ") +
                tr("the chemical explosive mode analysis (CEMA). The data ") +
                tr("can be understood as binning alpha values to classify ") +
                tr("the combustion modes (auto ignition, assisted ignition, ") +
                tr("and extinction zone)."));
        ui->image->hide();
    } else {
        ui->title->setText("Take A Break!");
        ui->content->setPlainText(tr("Click next when you are ready."));
        ui->image->hide();
    }
}

void UserStudyCtrlView::nextButtonClicked() {
    emit toPage(_pageId + 1);
}
