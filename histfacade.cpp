#include "histfacade.h"
#include <QOpenGLFunctions_3_3_Core>
#include <data/Histogram.h>
#include <yygl/gltexture.h>
#include <yygl/glvector.h>

/**
 * @brief HistFacade::create
 * @param hist
 * @param vars
 * @return
 */
std::shared_ptr<HistFacade> HistFacade::create(
        std::shared_ptr<const Hist> hist, std::vector<std::string> vars) {
    assert(std::vector<std::string>::size_type(hist->nDim()) == vars.size());
    if (3 == hist->nDim()) {
        auto hist3d = std::static_pointer_cast<const Hist3D>(hist);
        std::array<std::string,3> array = {{ vars[0], vars[1], vars[2] }};
        return std::make_shared<Hist3DFacade>(hist3d, array);
    }
    if (2 == hist->nDim()) {
        auto hist2d = std::static_pointer_cast<const Hist2D>(hist);
        std::array<std::string,2> array = {{ vars[0], vars[1] }};
        return std::make_shared<Hist2DFacade>(hist2d, array);
    }
    if (1 == hist->nDim()) {
        auto hist1d = std::static_pointer_cast<const Hist1D>(hist);
        return std::make_shared<Hist1DFacade>(hist1d, vars[0]);
    }
    if (0 == hist->nDim()) {
        return std::make_shared<HistNullFacade>();
    }
    return nullptr;
}

bool HistFacade::checkRange(
        const std::vector<Interval<float> > &intervals, float threshold) const {
    return hist()->checkRange(intervals, threshold);
}

std::shared_ptr<const Hist> HistFacade::hist(
        const std::vector<int> &dims) const {
    try {
        if (0 < _cachedHists.count(dims)) {
            return _cachedHists.at(dims);
        }
    } catch (...) {
        std::cout << "HistFacade::hist" << std::endl;
    }
    HistCollapser collapser(this->hist());
    std::shared_ptr<const Hist> hist = collapser.collapseTo(dims);
    _cachedHists[dims] = hist;
    return hist;
}

std::shared_ptr<const Hist> HistFacade::hist(
        const std::vector<std::string> &vars) const {
    return hist(varsToDims(vars));
}

std::shared_ptr<yy::gl::texture> HistFacade::texture(
        const std::vector<int> &dims) const
{
    try {
        if (0 < _cachedTextures.count(dims)) {
            return _cachedTextures.at(dims);
        }
    } catch (...) {
        std::cout << "HistFacade::texture" << std::endl;
    }
    /// TODO: right now only supports 2d histograms.
    assert(dims.size() == 2);
    auto hist = this->hist(dims);
    /// TODO: remove this for loop after switching to float from double for the
    /// histograms.
    std::vector<float> freqs(hist->nBins());
    for (auto iBin = 0; iBin < hist->nBins(); ++iBin) {
        freqs[iBin] = hist->binPercent(iBin);
    }
    auto texture = std::make_shared<yy::gl::texture>();
    texture->setWrapMode(
            yy::gl::texture::TEXTURE_2D, yy::gl::texture::CLAMP_TO_EDGE);
    texture->setTextureMinMagFilter(yy::gl::texture::TEXTURE_2D,
            yy::gl::texture::MIN_NEAREST, yy::gl::texture::MAG_NEAREST);
    texture->texImage2D(
            yy::gl::texture::TEXTURE_2D, yy::gl::texture::INTERNAL_R32F,
            hist->dim()[0], hist->dim()[1], yy::gl::texture::FORMAT_RED,
            yy::gl::texture::FLOAT, freqs.data());
    _cachedTextures[dims] = texture;
    return texture;
}

std::shared_ptr<yy::gl::texture> HistFacade::texture(
        const std::vector<std::string> &vars) const {
    return texture(varsToDims(vars));
}

/// TODO: instead of returning yy::gl::vector, which wastes memory, return
/// yy::gl::vertex_attrib instead. For this to happen, the shared_ptr of
/// yy::gl::buffer will have to be stored in another array.
std::shared_ptr<yy::gl::vector<float>> HistFacade::vbo(
        const std::vector<int> &dims) const {
    try {
        if (0 < _cachedVBOs.count(dims)) {
            return _cachedVBOs.at(dims);
        }
    } catch (...) {
        std::cout << "HistFacade::vbo" << std::endl;
    }
    auto hist = this->hist(dims);
    auto freqsPtr = std::make_shared<yy::gl::vector<float>>(hist->nBins());
    auto& freqs = *freqsPtr;
    for (auto iBin = 0; iBin < hist->nBins(); ++iBin) {
        freqs[iBin] = hist->binPercent(iBin);
//        freqs[iBin] = hist->values()[iBin];
    }
    _cachedVBOs[dims] = freqsPtr;
    return freqsPtr;
}

std::shared_ptr<yy::gl::vector<float>> HistFacade::vbo(
        const std::vector<std::string> &vars) const {
    return vbo(varsToDims(vars));
}

std::vector<int> HistFacade::varsToDims(
        const std::vector<std::string> &vars) const {
    std::vector<int> dims(vars.size());
    const auto& tVars = this->vars();
    for (unsigned int i = 0; i < vars.size(); ++i) {
        const auto& var = vars[i];
        auto itr = std::find(tVars.begin(), tVars.end(), var);
        assert(tVars.end() != itr);
        dims[i] = itr - tVars.begin();
    }
    return dims;
}

std::vector<std::string> HistFacade::dimsToVars(
        const std::vector<int> &dims) const {
    std::vector<std::string> vars(dims.size());
    const auto& tVars = this->vars();
    for (unsigned int i = 0; i < dims.size(); ++i) {
        vars[i] = tVars[dims[i]];
    }
    return vars;
}

/**
 * @brief Hist3DFacade::hist
 * @return
 */
std::vector<std::string> Hist3DFacade::vars() const {
    return { _vars[0], _vars[1], _vars[2] };
}

std::shared_ptr<const Hist> Hist3DFacade::hist() const {
    return _hist3d;
}

/**
 * @brief Hist2DFacade::hist
 * @return
 */
std::vector<std::string> Hist2DFacade::vars() const {
    return { _vars[0], _vars[1] };
}

std::shared_ptr<const Hist> Hist2DFacade::hist() const {
    return _hist2d;
}

/**
 * @brief Hist1DFacade::hist
 * @return
 */
std::shared_ptr<const Hist> Hist1DFacade::hist() const {
    return _hist1d;
}
