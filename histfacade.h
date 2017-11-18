#ifndef HISTFACADE_H
#define HISTFACADE_H

#include <QObject>
#include <memory>
#include <array>
#include <vector>
#include <string>
#include <data/Histogram.h>
#include <yygl/glvector.h>

namespace yy {
namespace gl {
class texture;
class buffer;
} // namespace gl
} // namespace yy

/**
 * @brief The HistFacade class
 */
class HistFacade : public QObject {
    Q_OBJECT
public:
    static std::shared_ptr<HistFacade> create(
            std::shared_ptr<const Hist> hist, std::vector<std::string> vars);

public:
    virtual ~HistFacade() {}
    virtual int nDim() const { return vars().size(); }
    virtual std::vector<std::string> vars() const = 0;
    virtual bool selected() const = 0;
    virtual void setSelected(bool selected) = 0;
    virtual bool checkRange(const std::vector<Interval<float>>& intervals,
            float threshold) const;

public:
    virtual std::shared_ptr<const Hist> hist() const = 0;
    virtual std::shared_ptr<const Hist> hist(
            const std::vector<int>& dims) const;
    virtual std::shared_ptr<const Hist> hist(
            const std::array<int, 2>& dims) const {
        return hist(std::vector<int>{{ dims[0], dims[1] }});
    }
    virtual std::shared_ptr<const Hist> hist(int dim) const {
        return hist(std::vector<int>{ dim });
    }
    virtual std::shared_ptr<const Hist> hist(
            const std::vector<std::string>& vars) const;

public:
    virtual std::shared_ptr<yy::gl::texture> texture(
            const std::vector<int>& dims) const;
    virtual std::shared_ptr<yy::gl::texture> texture(
            const std::array<int, 2>& dims) const {
        return texture(std::vector<int>{{ dims[0], dims[1] }});
    }
    virtual std::shared_ptr<yy::gl::texture> texture(
            const std::vector<std::string>& vars) const;

public:
    virtual std::shared_ptr<yy::gl::vector<float>> vbo(
            const std::vector<int>& dims) const;
    virtual std::shared_ptr<yy::gl::vector<float>> vbo(int dim) const {
        return vbo(std::vector<int>{ dim });
    }
    virtual std::shared_ptr<yy::gl::vector<float>> vbo(
            const std::vector<std::string>& vars) const;

public:
    virtual std::array<double, 2> dimRange(int iDim) const {
        return {hist()->dimMin(iDim), hist()->dimMax(iDim)};
    }

protected:
    std::vector<int> varsToDims(const std::vector<std::string>& vars) const;
    std::vector<std::string> dimsToVars(const std::vector<int>& dims) const;

private:
    template <typename T>
    using FacadeMap = std::map<std::vector<int>, T>;
    mutable FacadeMap<std::shared_ptr<const Hist>> _cachedHists;
    mutable FacadeMap<std::shared_ptr<yy::gl::texture>> _cachedTextures;
    mutable FacadeMap<std::shared_ptr<yy::gl::vector<float>>> _cachedVBOs;
};

/**
 * @brief The HistNullFacade class
 */
class HistNullFacade : public HistFacade {
    Q_OBJECT
public:
    virtual bool selected() const override { return false; }
    virtual void setSelected(bool) override {}
    virtual std::vector<std::string> vars() const override {
        static std::vector<std::string> v;
        return v;
    }
    virtual std::shared_ptr<const Hist> hist() const override {
        static std::shared_ptr<HistNull> h = std::make_shared<HistNull>();
        return h;
    }
    virtual std::shared_ptr<const Hist> hist(
            const std::vector<int>& dims) const override {
        return hist();
    }
};

/**
 * @brief The Hist3DFacade class
 */
class Hist3DFacade : public HistFacade {
    Q_OBJECT
public:
    Hist3DFacade(std::shared_ptr<const Hist3D> hist3d,
            std::array<std::string,3> vars)
      : _hist3d(hist3d), _vars(vars), _selected(true) {}

public:
    virtual bool selected() const override { return _selected; }
    virtual void setSelected(bool selected) override { _selected = selected; }
    virtual std::vector<std::string> vars() const override;
    virtual std::shared_ptr<const Hist> hist() const override;

private:
    std::shared_ptr<const Hist3D> _hist3d;
    std::array<std::string,3> _vars;
    bool _selected;
};



/**
 * @brief The Hist2DFacade class
 */
class Hist2DFacade : public HistFacade {
    Q_OBJECT
public:
    Hist2DFacade(std::shared_ptr<const Hist2D> hist2d,
            std::array<std::string,2> vars)
      : _hist2d(hist2d), _vars(vars), _selected(true) {}

public:
    virtual bool selected() const override { return _selected; }
    virtual void setSelected(bool selected) override { _selected = selected; }
    virtual std::vector<std::string> vars() const override;
    virtual std::shared_ptr<const Hist> hist() const override;

private:
    std::shared_ptr<const Hist2D> _hist2d;
    std::array<std::string,2> _vars;
    bool _selected;
};

/**
 * @brief The Hist1DFacade class
 */
class Hist1DFacade : public HistFacade {
    Q_OBJECT
public:
    Hist1DFacade(std::shared_ptr<const Hist1D> hist1d, std::string var)
      : _hist1d(hist1d), _var(var), _selected(true) {}

public:
    virtual bool selected() const override { return _selected; }
    virtual void setSelected(bool selected) override { _selected = selected; }
    virtual std::vector<std::string> vars() const override { return { _var }; }
    virtual std::shared_ptr<const Hist> hist() const override;

private:
    std::shared_ptr<const Hist1D> _hist1d;
    std::string _var;
    bool _selected;
};

#endif // HISTFACADE_H
