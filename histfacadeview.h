#ifndef HISTFACADEVIEW_H
#define HISTFACADEVIEW_H

#include <widget.h>
#include <vector>
#include <memory>

class HistFacade;
class HistView;
class HistDimsCombo;

class HistFacadeView : public Widget {
    Q_OBJECT
public:
    explicit HistFacadeView(QWidget* parent);

public:
    void setHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);
    void setHistFacade(std::shared_ptr<const HistFacade> histFacade);

private:
    std::shared_ptr<const HistFacade> _histFacade;
    std::vector<int> _displayDims;
    HistDimsCombo* _displayDimsCombo;
    HistView* _histView;
};

#endif // HISTFACADEVIEW_H
