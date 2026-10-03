#pragma once
#include "document/document.h"
#include <Graphic3d_Mat4d.hxx>
#include <QRectF>
#include <QSizeF>
namespace lmx {
struct CameraModel {
    std::array<double, 3> position, target, forward, right, up;
    double lens, sensorWidth, shiftX, shiftY, clipNear, clipFar, fstop, focusDistance;
};
CameraModel cameraModel(const Entity &camera);
QRectF cameraFrame(const QSizeF &viewport, const QSizeF &image);
Graphic3d_Mat4d cameraProjection(const CameraModel &camera, const QSizeF &viewport, const QSizeF &image);
// Normalized image coordinates: origin at bottom left, as in Blender's world_to_camera_view.
std::array<double, 3> cameraPoint(const CameraModel &camera, const QSizeF &image,
                                  const std::array<double, 3> &world);
} // namespace lmx
