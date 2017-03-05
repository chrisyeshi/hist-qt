#ifndef CAMERA_H
#define CAMERA_H

#undef near
#undef far

#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>
#include <memory>

class ICamera
{
public:
    enum ProjMode { PM_Perspective, PM_Orthographic };

public:
    virtual const QMatrix4x4& matView() const = 0;
    virtual const QMatrix4x4& matProj() const = 0;
};

class CameraCore : public ICamera
{
public:
    CameraCore();
    virtual ~CameraCore() {}

public:
    virtual const QMatrix4x4& matView() const;
    virtual const QMatrix4x4& matProj() const;

public:
    QVector3D eye() const { return _eye; }
    void setEye(const QVector3D& eye) { _eye = eye; }
    QVector3D center() const { return _center; }
    void setCenter(const QVector3D& center) { _center = center; }
    QVector3D up() const { return _up; }
    void setUp(const QVector3D& up) { _up = up; }
    ProjMode projMode() const { return _projMode; }
    void setProjMode(const ProjMode &projMode) { _projMode = projMode; }
    float fov() const { return _fov; }
    void setFOV(float fov) { _fov = fov; }
    float aspectRatio() const { return _aspect; }
    void setAspectRatio(const float ratio) { _aspect = ratio; }
    float near() const { return _near; }
    void setNear(float near) { _near = near; }
    float far() const { return _far; }
    void setFar(float far) { _far = far; }
    void setNearFar(float near, float far) { setNear(near); setFar(far); }

public:
    QVector2D focalPlaneSize() const;

private:
    QVector3D _eye;
    QVector3D _center;
    QVector3D _up;
    ProjMode _projMode;
    float _fov, _aspect;
    float _near, _far;
};

/// TODO: more camera models

class Camera : public ICamera
{
public:
    Camera();

public:
    virtual const QMatrix4x4& matView() const { return m_core->matView(); }
    virtual const QMatrix4x4& matProj() const { return m_core->matProj(); }

public:
    QVector3D eye() const { return m_core->eye(); }
    void setEye(const QVector3D& eye) { m_core->setEye(eye); }
    QVector3D center() const { return m_core->center(); }
    void setCenter(const QVector3D& center) { m_core->setCenter(center); }
    QVector3D up() const { return m_core->up(); }
    void setUp(const QVector3D& up) { m_core->setUp(up); }
    ProjMode projMode() const { return m_core->projMode(); }
    void setProjMode(const ProjMode &mode) { m_core->setProjMode(mode); }
    float fov() const { return m_core->fov(); }
    void setFOV(float fov) { m_zoom = fov2zoom(fov); m_core->setFOV(fov); }
    float aspectRatio() const { return m_core->aspectRatio(); }
    void setAspectRatio(const float ratio) { m_core->setAspectRatio(ratio); }
    float near() const;
    float nearRange() const { return m_nearRange; }
    float far() const;
    float farRange() const { return m_farRange; }

public:
    void orbit(const QVector2D& dir);
    void track(const QVector2D& dir);
    void zoom(const float factor);
    void reset(const QVector3D& bmin, const QVector3D& bmax);
    void resetNearFar(const QVector3D& bmin, const QVector3D& bmax);
    void setNearFactor(float nearFactor);
    void setFarFactor(float farFactor);
    void setNearFarFactor(float nearFactor, float farFactor);

private:
    float zoom2fov(float zoom) const;
    float fov2zoom(float fov) const;

private:
    const float initZoom = -0.5;

private:
    std::shared_ptr<CameraCore> m_core;
    float m_zoom;
    float m_nearRange, m_farRange, m_nearFactor, m_farFactor;
};

#endif // CAMERA_H
