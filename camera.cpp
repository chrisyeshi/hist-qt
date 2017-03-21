#include "camera.h"
#include <iostream>
#include <limits>
#include <QVector2D>
#include <QMatrix3x3>
#include <QtMath>

/**
 * @brief CameraCore::CameraCore
 */
CameraCore::CameraCore()
  : _eye(0.f, 0.f, 4.f)
  , _center(0.f, 0.f, 0.f)
  , _up(0.f, 1.f, 0.f)
  , _projMode(PM_Perspective)
  , _fov(60.f)
  , _aspect(1.f)
  , _near(0.001f), _far(1000.f)
{}

const QMatrix4x4& CameraCore::matView() const
{
    static QMatrix4x4 mat;
    mat.setToIdentity();
    mat.lookAt(_eye, _center, _up);
    return mat;
}

const QMatrix4x4& CameraCore::matProj() const
{
    static QMatrix4x4 mat;
    mat.setToIdentity();
    if (PM_Perspective == _projMode)
    {
        mat.perspective(_fov, _aspect, _near, _far);
    } else
    {
        QVector2D size = focalPlaneSize() / 2.f;
        mat.ortho(-size.x(), size.x(), -size.y(), size.y(), _near, _far);
    }
    return mat;
}

QVector2D CameraCore::focalPlaneSize() const
{
    float viewLen = (_center - _eye).length();
    float height = qTan(_fov / 2.f / 180.f * M_PI) * viewLen;
    return QVector2D(height * _aspect * 2.f, height * 2.f);
}

/**
 * @brief Camera::Camera
 */
Camera::Camera()
  : m_core(new CameraCore())
  , m_zoom(initZoom)
  , m_nearRange(0.f), m_farRange(1.f)
  , m_nearFactor(0.f), m_farFactor(1.f)
{
    m_core->setNearFar(near(), far());
}

float Camera::near() const
{
    float range = m_farRange - m_nearRange;
    return m_nearFactor * range + m_nearRange;
}

float Camera::far() const
{
    float range = m_farRange - m_nearRange;
    return m_farFactor * range + m_nearRange;
}

void Camera::orbit(const QVector2D &dir)
{
    QVector3D eye = m_core->eye();
    QVector3D center = m_core->center();
    QVector3D up = m_core->up();
    QVector3D u = (center - eye).normalized();
    QVector3D s = QVector3D::crossProduct(u, up).normalized();
    QVector3D t = QVector3D::crossProduct(s, u).normalized();
    QVector3D rotateDir = (dir.x() * s + dir.y() * t).normalized();
    QVector3D rotateAxis = QVector3D::crossProduct(rotateDir, u).normalized();
    float angle = dir.length() * 360.f;
    QMatrix4x4 matRotate;
    matRotate.rotate(-angle, rotateAxis);
    QVector3D view = matRotate * (eye - center);
    eye = center + view;
    up = QVector3D::crossProduct(view, s).normalized();
    m_core->setEye(eye);
    m_core->setUp(up);
}

void Camera::track(const QVector2D &dir)
{
    QVector3D eye = m_core->eye();
    QVector3D center = m_core->center();
    QVector3D up = m_core->up();
    QVector3D u = (center - eye).normalized();
    QVector3D s = QVector3D::crossProduct(u, up).normalized();
    QVector3D t = QVector3D::crossProduct(s, u).normalized();
    QVector2D scaleDir = dir * m_core->focalPlaneSize();
    QVector3D trackDir = (scaleDir.x() * s + scaleDir.y() * t);
    eye = eye - trackDir;
    center = center - trackDir;
    m_core->setEye(eye);
    m_core->setCenter(center);
}

void Camera::zoom(const float factor)
{
    m_zoom += factor / 1000.f;
    m_core->setFOV(zoom2fov(m_zoom));
}

void Camera::reset(const QVector3D &bmin, const QVector3D &bmax)
{
    // reset the camera to at the z directon
    // reset the object to the center of the screen
    // calculate the distance from center so that everything fits in the screen
    m_core->setCenter((bmin + bmax) / 2.f);
    m_core->setUp(QVector3D(0.f, 1.f, 0.f));
    m_zoom = initZoom;
    m_core->setFOV(zoom2fov(m_zoom));
    float diagonal = (bmax - bmin).length();
    float distance = (diagonal / 2.f) / tan(zoom2fov(m_zoom) / 180.f * M_PI / 2.f);
    distance = distance + bmax.z() - m_core->center().z();
    m_core->setEye(m_core->center() - QVector3D(0.f, 0.f, 1.f) * distance);
    resetNearFar(bmin, bmax);
}

void Camera::resetNearFar(const QVector3D &bmin, const QVector3D &bmax)
{
    // only reset the near and far plane so that the volume remains in the frustum
    // get the 8 corners
    std::vector<QVector3D> corners
            = { QVector3D(bmin.x(), bmin.y(), bmin.z()),
                QVector3D(bmax.x(), bmin.y(), bmin.z()),
                QVector3D(bmin.x(), bmax.y(), bmin.z()),
                QVector3D(bmax.x(), bmax.y(), bmin.z()),
                QVector3D(bmin.x(), bmin.y(), bmax.z()),
                QVector3D(bmax.x(), bmin.y(), bmax.z()),
                QVector3D(bmin.x(), bmax.y(), bmax.z()),
                QVector3D(bmax.x(), bmax.y(), bmax.z()) };
    m_nearRange = std::numeric_limits<float>::max();
    m_farRange  = 0.1f;
    for (auto c : corners)
    {
        float distance = QVector3D::dotProduct(c - m_core->eye(), (m_core->center() - m_core->eye()).normalized());
        m_nearRange = std::min(m_nearRange, distance);
        m_farRange  = std::max(m_farRange,  distance);
    }
    m_nearRange = std::max(m_nearRange, 0.1f) - 0.01f;
    m_farRange += 0.01f;
    m_core->setNearFar(near(), far());
}

void Camera::setNearFactor(float nearFactor)
{
    m_nearFactor = nearFactor;
    m_core->setNear(near());
}

void Camera::setFarFactor(float farFactor)
{
    m_farFactor = farFactor;
    m_core->setFar(far());
}

void Camera::setNearFarFactor(float nearFactor, float farFactor)
{
    this->setNearFactor(nearFactor);
    this->setFarFactor(farFactor);
}

float Camera::zoom2fov(float zoom) const
{
    return (qAtan(zoom) / M_PI + 0.5f) * 180.f;
}

float Camera::fov2zoom(float fov) const
{
    return qTan((fov / 180.f - 0.5f) * M_PI);
}
