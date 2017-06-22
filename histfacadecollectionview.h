#ifndef HISTFACADECOLLECTIONVIEW_H
#define HISTFACADECOLLECTIONVIEW_H

#include <widget.h>
#include <QVector>

class QHBoxLayout;
class HistFacade;
class HistFacadeView;

class HistFacadeCollectionView : public Widget {
    Q_OBJECT
public:
    explicit HistFacadeCollectionView(
            QWidget* parent, Qt::WindowFlags flags = Qt::WindowFlags());

public:
    void appendHist(std::shared_ptr<const HistFacade> histFacade,
            std::vector<int> displayDims);

private:
    QHBoxLayout* _layout;
    QVector<HistFacadeView*> _histFacadeViews;
};

#endif // HISTFACADECOLLECTIONVIEW_H
