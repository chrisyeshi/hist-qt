#ifndef HISTVOLUMEPHYSICALVIEW_H
#define HISTVOLUMEPHYSICALVIEW_H

#include <histvolumeview.h>
#include <openglwidget.h>
#include <yygl/gltexture.h>

class HistView;
class HistVolumePhysicalOpenGLView;

/**
 * @brief The HistVolumePhysicalView class
 */
class HistVolumePhysicalView : public HistVolumeView {
public:
    HistVolumePhysicalView(QWidget* parent = nullptr);

public:
    virtual void update() override;
    virtual void setHistConfigs(std::vector<HistConfig> configs) override;
    virtual void setDataStep(std::shared_ptr<DataStep> dataStep) override;

private:
    std::string currHistName() const;

private:
    HistView* _histView;
    HistVolumePhysicalOpenGLView* _histVolumeView;
    std::vector<HistConfig> _histConfigs;
    std::vector<int> _histDims;
    std::shared_ptr<DataStep> _dataStep;
    int _currHistId = 0;
    int _currHistConfigId = 0;
};

/**
 * @brief The HistVolumePhysicalOpenGLView class
 */
class HistVolumePhysicalOpenGLView : public OpenGLWidget {
public:
//    class ScalarVolume {
//        using namespace yy;
//    public:
//        ScalarVolume() = default;
//        ScalarVolume(
//                std::vector<float> buffer, int width, int height, int depth);
//        void setBuffer(
//                std::vector<float> buffer, int width, int height, int depth);

//    public:
//        float min() const { return _min; }
//        float max() const { return _max; }
//        const gl::texture& texture() const { return _texture; }
//        gl::texture::INTERNAL_FORMAT internalFormat() const {
//            return gl::texture::INTERNAL_R32F;
//        }
//        int width() const { return _width; }
//        int height() const { return _height; }
//        int depth() const { return _depth; }
//        gl::texture::FORMAT format() const { return gl::texture::FORMAT_RED; }
//        gl::texture::DATA_TYPE type() const { return gl::texture::FLOAT; }

//    private:
//        void update();

//    private:
//        float _min = 0.f, _max = 0.f;
//        int _width = 0, _height = 0, _depth = 0;
//        gl::texture _texture;
//        std::vector<float> _buffer;
//    };

public:
    HistVolumePhysicalOpenGLView(QWidget* parent = nullptr);

public:
    void setHistVolume(std::shared_ptr<HistFacadeVolume> histVolume);

protected:
    void resizeGL(int w, int h);
    void paintGL();

private:
    std::shared_ptr<HistFacadeVolume> _histVolume;
//    std::vector<std::shared_ptr<ScalarVolume>> _avgVolumes;
};

#endif // HISTVOLUMEPHYSICALVIEW_H
