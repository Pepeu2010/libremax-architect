#include "camera_model.h"
#include <cmath>
#include <stdexcept>
namespace lmx {
namespace {
using Vector = std::array<double, 3>;
double dot(const Vector &a, const Vector &b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Vector cross(const Vector &a, const Vector &b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
Vector normalized(Vector v) {
    const auto length = std::sqrt(dot(v, v));
    if (!std::isfinite(length) || length < 1e-8)
        throw std::invalid_argument(
            "A câmera precisa apontar para outro ponto e ter uma direção vertical válida.");
    for (auto &value : v)
        value /= length;
    return v;
}
Vector vector(const Json &parameters, const char *key, const Json &fallback) {
    const auto value = parameters.value(key, fallback);
    if (!value.is_array() || value.size() != 3)
        throw std::invalid_argument("A posição, o alvo e a direção vertical precisam ter três coordenadas.");
    return value.get<Vector>();
}
double scalar(const Json &parameters, const char *key, double fallback, double minimum, double maximum) {
    const auto value = parameters.value(key, fallback);
    if (!std::isfinite(value) || value < minimum || value > maximum)
        throw std::invalid_argument("Confira lente, enquadramento e distâncias da câmera.");
    return value;
}
void size(const QSizeF &value) {
    if (!std::isfinite(value.width()) || !std::isfinite(value.height()) || value.width() <= 0 ||
        value.height() <= 0)
        throw std::invalid_argument("O enquadramento precisa ter largura e altura válidas.");
}
} // namespace
CameraModel cameraModel(const Entity &e) {
    if (e.type != "Camera")
        throw std::invalid_argument("Escolha uma câmera para ver o enquadramento.");
    CameraModel result{};
    result.position = {e.transform.x, e.transform.y, e.transform.z};
    result.target = vector(e.parameters, "target", Json::array({2000, 1500, 1000}));
    Vector direction;
    for (int axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(result.position[axis]) || !std::isfinite(result.target[axis]))
            throw std::invalid_argument("A posição e o alvo da câmera precisam ser válidos.");
        direction[axis] = result.target[axis] - result.position[axis];
    }
    const auto distance = std::sqrt(dot(direction, direction));
    result.forward = normalized(direction);
    auto up = vector(e.parameters, "up", Json::array({0, 0, 1}));
    if (!e.parameters.contains("up") && std::abs(result.forward[2]) > .999999)
        up = {0, 1, 0};
    result.right = normalized(cross(result.forward, normalized(up)));
    result.up = normalized(cross(result.right, result.forward));
    result.lens = scalar(e.parameters, "lens", 28, 1, 1000);
    result.sensorWidth = scalar(e.parameters, "sensorWidth", 36, 1, 100);
    result.shiftX = scalar(e.parameters, "shiftX", 0, -2, 2);
    result.shiftY = scalar(e.parameters, "shiftY", 0, -2, 2);
    result.clipNear = scalar(e.parameters, "clipNear", 100, .1, 1e8);
    result.clipFar = scalar(e.parameters, "clipFar", 1e6, .1, 1e8);
    if (result.clipFar <= result.clipNear)
        throw std::invalid_argument("A distância máxima da câmera precisa ser maior que a mínima.");
    result.fstop = scalar(e.parameters, "fstop", 8, 1, 64);
    result.focusDistance = e.parameters.contains("focusDistance")
                               ? scalar(e.parameters, "focusDistance", distance, 100, 1e7)
                               : distance;
    return result;
}
QRectF cameraFrame(const QSizeF &viewport, const QSizeF &image) {
    size(viewport);
    size(image);
    const double scale = std::min(viewport.width() / image.width(), viewport.height() / image.height());
    const auto frame = image * scale;
    return {(viewport.width() - frame.width()) / 2, (viewport.height() - frame.height()) / 2, frame.width(),
            frame.height()};
}
Graphic3d_Mat4d cameraProjection(const CameraModel &c, const QSizeF &viewport, const QSizeF &image) {
    const auto frame = cameraFrame(viewport, image);
    const double halfWidth = c.sensorWidth / (2 * c.lens),
                 halfHeight = halfWidth * image.height() / image.width();
    const double visibleWidth = halfWidth * viewport.width() / frame.width(),
                 visibleHeight = halfHeight * viewport.height() / frame.height();
    Graphic3d_Mat4d matrix;
    matrix.InitZero();
    matrix.SetValue(0, 0, 1 / visibleWidth);
    matrix.SetValue(1, 1, 1 / visibleHeight);
    matrix.SetValue(0, 2, 2 * halfWidth * c.shiftX / visibleWidth);
    matrix.SetValue(1, 2, 2 * halfWidth * c.shiftY / visibleHeight);
    matrix.SetValue(2, 2, -(c.clipFar + c.clipNear) / (c.clipFar - c.clipNear));
    matrix.SetValue(2, 3, -2 * c.clipFar * c.clipNear / (c.clipFar - c.clipNear));
    matrix.SetValue(3, 2, -1);
    return matrix;
}
std::array<double, 3> cameraPoint(const CameraModel &c, const QSizeF &image, const Vector &world) {
    size(image);
    Vector delta;
    for (int axis = 0; axis < 3; ++axis)
        delta[axis] = world[axis] - c.position[axis];
    const auto distance = dot(delta, c.forward);
    if (std::abs(distance) < 1e-8)
        throw std::invalid_argument("O ponto está no plano da câmera.");
    const double halfWidth = c.sensorWidth / (2 * c.lens), aspect = image.width() / image.height();
    return {.5 + dot(delta, c.right) / (2 * distance * halfWidth) - c.shiftX,
            .5 + dot(delta, c.up) * aspect / (2 * distance * halfWidth) - c.shiftY * aspect, distance / 1000};
}
} // namespace lmx
