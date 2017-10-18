#include "histvolumephysicalview.h"
#include <QBoxLayout>
#include <QLabel>
#include <histview.h>

namespace {

} // unnamed namespace

/**
 * @brief HistVolumePhysicalView::HistVolumePhysicalView
 * @param parent
 */
HistVolumePhysicalView::HistVolumePhysicalView(QWidget *parent)
      : HistVolumeView(parent)
      , _histView(new HistView(this))
      , _histVolumeView(new HistVolumePhysicalOpenGLView(this)) {
    auto hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(0);
    hLayout->addWidget(_histView);
    hLayout->addWidget(_histVolumeView);
}

void HistVolumePhysicalView::update() {
    _histView->setHist(
            _dataStep->smartVolume(currHistName())->hist(_currHistId),
            _histDims);
}

void HistVolumePhysicalView::setHistConfigs(std::vector<HistConfig> configs) {
    _histConfigs = configs;
    _currHistConfigId = 0;
    _histDims = {0};
}

void HistVolumePhysicalView::setDataStep(std::shared_ptr<DataStep> dataStep) {
    _dataStep = dataStep;
}

std::string HistVolumePhysicalView::currHistName() const {
    return _histConfigs[_currHistId].name();
}

///**
// * @brief HistVolumePhysicalOpenGLView::ScalarVolume::ScalarVolume
// * @param buffer
// * @param width
// * @param height
// * @param depth
// */
//HistVolumePhysicalOpenGLView::ScalarVolume::ScalarVolume(
//        std::vector<float> buffer, int width, int height, int depth)
//  : _buffer(buffer), _width(width), _height(height), _depth(depth) {
//    update();
//}

//void HistVolumePhysicalOpenGLView::ScalarVolume::setBuffer(
//        std::vector<float> buffer, int width, int height, int depth) {
//    _buffer = buffer;
//    _width = width;
//    _height = height;
//    _depth = depth;
//    update();
//}

//void HistVolumePhysicalOpenGLView::ScalarVolume::update() {
//    _texture.texImage3D(internalFormat(), width(), height(), depth(),
//                        format(), type(), _buffer.data());
//    _min = std::numeric_limits<float>::max();
//    _max = std::numeric_limits<float>::lowest();
//    for (auto val : _buffer) {
//        _min = std::min(val, _min);
//        _max = std::max(val, _max);
//    }
//}

/**
 * @brief HistVolumePhysicalOpenGLView::HistVolumePhysicalOpenGLView
 * @param parent
 */
HistVolumePhysicalOpenGLView::HistVolumePhysicalOpenGLView(QWidget *parent)
      : OpenGLWidget(parent) {

}

void HistVolumePhysicalOpenGLView::setHistVolume(
        std::shared_ptr<HistFacadeVolume> histVolume) {
    _histVolume = histVolume;
    delayForInit([this]() {
//        int nVar = _histVolume->vars().size();
//        _avgVolumes.resize(nVar);
//        for (int iVar = 0; iVar < nVar; ++iVar) {
//            auto dimHists = _histVolume->dimHists();
//            auto nHist = dimHists[0] * dimHists[1] * dimHists[2];
//            std::vector<float> avgBuffer(nHist);
//            for (int z = 0; z < dimHists[2]; ++z)
//            for (int y = 0; y < dimHists[1]; ++y)
//            for (int x = 0; x < dimHists[0]; ++x) {
//                int iHist = dimHists.idstoflat(x, y, z);
//                avgBuffer[iHist] = calcAverage(_histVolume->hist(iHist), iVar);
//            }
//            _avgVolumes[iVar]->setBuffer(
//                    avgBuffer, dimHists[0], dimHists[1], dimHists[2]);
//        }
    });
}

void HistVolumePhysicalOpenGLView::resizeGL(int w, int h) {

}

void HistVolumePhysicalOpenGLView::paintGL() {

}
