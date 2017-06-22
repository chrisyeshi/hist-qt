#include "histfacadeview.h"
#include <histfacade.h>
#include <histview.h>
#include <histdimscombo.h>
#include <QVBoxLayout>
#include <QComboBox>

HistFacadeView::HistFacadeView(QWidget *parent)
  : Widget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(4);
    _displayDimsCombo = new HistDimsCombo(this);
    connect(_displayDimsCombo, &HistDimsCombo::dimsChanged,
            this, [this](std::vector<int> displayDims) {
        _displayDims = displayDims;
        _histView->setHist(_histFacade, _displayDims);
        _histView->update();
    });
    _histView = new HistView(this);
    layout->addWidget(_displayDimsCombo, 0);
    layout->addWidget(_histView, 1);
}

void HistFacadeView::setHist(std::shared_ptr<const HistFacade> histFacade,
        std::vector<int> displayDims) {
    _histFacade = histFacade;
    _displayDims = displayDims;
    // select the right item in the combobox
    _displayDimsCombo->blockSignals(true);
    _displayDimsCombo->setItems(_histFacade->vars());
    _displayDimsCombo->setCurrentDims(_displayDims);
    _displayDimsCombo->blockSignals(false);
    // update the histview
    _histView->setHist(_histFacade, _displayDims);
    _histView->update();
}

void HistFacadeView::setHistFacade(
        std::shared_ptr<const HistFacade> histFacade) {
    /// TODO: populate the combobox
    /// TODO: select the first item in the combobox
    /// TODO: update the histview
}
