#include "histfacadecollectionview.h"
#include <histfacadeview.h>
#include <QHBoxLayout>

HistFacadeCollectionView::HistFacadeCollectionView(
        QWidget *parent, Qt::WindowFlags flags)
    : Widget(parent, flags) {
    _layout = new QHBoxLayout(this);
    _layout->setMargin(4);
    _layout->setSpacing(5);
}

void HistFacadeCollectionView::appendHist(
        std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    HistFacadeView* view = new HistFacadeView(this);
    view->setHist(histFacade, displayDims);
    _histFacadeViews.append(view);
    _layout->addWidget(view);
}
