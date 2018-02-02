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
                tr("Walk-through all the controls and explain the") +
                tr(" functionalities, including orientation,") +
                tr(" zooming and panning, frequency") +
                tr(" percentage range, histogram ranges (brush),") +
                tr(" histogram configs,") +
                tr(" merging histograms,") +
                tr(" histogram frequency reading,") +
                tr(" the relation between 1D and 2D histograms,") +
                tr(" and time steps."));
        ui->image->hide();
    } else if (2 == _pageId) {
        ui->title->setText("Training");
        ui->content->setPlainText(
                tr("Change the visible range of the vertical axis to") +
                tr(" hide frequency values that are below 20%."));
//                tr("Hide the bins that are lower than 20% in frequency ") +
//                "percentage.");
        ui->image->hide();
    } else if (4 == _pageId) {
        ui->title->setText("Training");
        ui->content->setPlainText(
                tr("What is the frequency percentage range of the ") +
                tr("current slice?"));
//                tr("Of all the histograms in the current slice, identify ") +
//                "the bin that has the highest frequency percentage in any " +
//                "histogram. What is the frequency percentage and which " +
//                "histogram is it?");
        ui->image->hide();
    } else if (6 == _pageId) {
        ui->title->setText("Training");
        ui->content->setPlainText(
                tr("Select an interesting 2D histogram at ") +
                "time step 36 and change the histogram ranges to only " +
                "show the most interesting portion.");
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
                tr("At the first XY slice of time step 16, select all ") +
                "the histograms that have a similar pattern within the same " +
                "histogram ranges.");
        ui->image->show();
    } else if (12 == _pageId) {
        ui->title->setText("User Study");
        ui->content->setPlainText(
                tr("Do not cancel the selection! With the current") +
                tr(" selection, refer to the merged histogram.") +
                tr(" Which single bin has the highest frequency? What is") +
                tr(" the bin frequency and the bin value range") +
                tr(" (for both heat release and CH2O)?"));
//                tr("Of the merged histogram from the selected histograms, ") +
//                "which single bin has the highest frequency? What is the " +
//                "frequency and histogram ranges of the bin?");
        ui->image->show();
    } else if (14 == _pageId) {
        ui->title->setText("User Study");
        ui->content->setPlainText(
                tr("Of the selected histograms, identify the one with") +
                tr(" the lowest heat release."));
        ui->image->hide();
    } else if (16 == _pageId) {
        ui->title->setText("User Study");
        ui->content->setPlainText(
                tr("Of the previously selected histogram, what is the") +
                tr(" frequency value within heat release range") +
                tr(" [-9e+9, -8e+9]?"));
    } else {
        ui->title->setText("Take A Break!");
        ui->content->setPlainText(tr("Click next when you are ready."));
        ui->image->hide();
    }
}

void UserStudyCtrlView::nextButtonClicked() {
    emit toPage(_pageId + 1);
}
