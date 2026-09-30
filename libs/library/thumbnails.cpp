#include "thumbnails.h"
#include "geometry/geometry.h"
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <QCryptographicHash>
#include <QDir>
#include <QFutureWatcher>
#include <QPainter>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtConcurrent>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <algorithm>
#include <cmath>

namespace lmx {
QImage renderAssetThumbnail(const Asset &asset) {
    Document document;
    auto object = Library::instantiate(asset, 0, 0);
    document.entities.push_back(object);
    document.validate();
    struct Triangle {
        QPolygonF points;
        QColor color;
        double depth;
    };
    std::vector<Triangle> triangles;
    QRectF bounds;
    bool first = true;
    for (const auto &part : buildEntity(document, object)) {
        BRepMesh_IncrementalMesh mesh(part.shape, 3.0, false, 0.5, false);
        if (!mesh.IsDone())
            throw std::runtime_error("Falha ao gerar miniatura");
        auto material = std::find_if(document.materials.begin(), document.materials.end(),
                                     [&](const auto &m) { return m.at("id") == part.material; });
        for (TopExp_Explorer faces(part.shape, TopAbs_FACE); faces.More(); faces.Next()) {
            auto face = TopoDS::Face(faces.Current());
            TopLoc_Location location;
            auto meshData = BRep_Tool::Triangulation(face, location);
            if (meshData.IsNull())
                continue;
            for (int i = 1; i <= meshData->NbTriangles(); ++i) {
                int a, b, c;
                meshData->Triangle(i).Get(a, b, c);
                if (face.Orientation() == TopAbs_REVERSED)
                    std::swap(b, c);
                auto p = meshData->Node(a).Transformed(location.Transformation());
                auto q = meshData->Node(b).Transformed(location.Transformation());
                auto r = meshData->Node(c).Transformed(location.Transformation());
                auto normal = gp_Vec(p, q).Crossed(gp_Vec(p, r));
                double shade = 0.55;
                if (normal.Magnitude() > 1e-8)
                    shade += 0.45 * std::max(0.0, normal.Normalized().Dot(gp_Vec(-0.4, 0.5, 1).Normalized()));
                const auto &color = material->at("baseColor");
                QColor tint;
                tint.setRgbF(std::pow(color[0].get<double>() * shade, 1 / 2.2),
                             std::pow(color[1].get<double>() * shade, 1 / 2.2),
                             std::pow(color[2].get<double>() * shade, 1 / 2.2));
                QPolygonF polygon;
                for (const auto &point : {p, q, r}) {
                    QPointF projected((point.X() + point.Y()) * 0.82,
                                      (point.Y() - point.X()) * 0.36 - point.Z());
                    polygon << projected;
                    if (first) {
                        bounds = QRectF(projected, QSizeF(0, 0));
                        first = false;
                    } else {
                        const auto left = std::min(bounds.left(), projected.x());
                        const auto right = std::max(bounds.right(), projected.x());
                        const auto top = std::min(bounds.top(), projected.y());
                        const auto bottom = std::max(bounds.bottom(), projected.y());
                        bounds = QRectF(QPointF(left, top), QPointF(right, bottom));
                    }
                }
                auto depth = [](const gp_Pnt &point) { return point.Y() - point.X() + point.Z(); };
                triangles.push_back({polygon, tint, depth(p) + depth(q) + depth(r)});
            }
        }
    }
    if (triangles.empty())
        throw std::runtime_error("Miniatura sem geometria");
    std::sort(triangles.begin(), triangles.end(),
              [](const auto &a, const auto &b) { return a.depth < b.depth; });
    QImage result(192, 144, QImage::Format_ARGB32_Premultiplied);
    result.fill(QColor("#111a22"));
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    const double scale =
        std::min(166.0 / std::max(bounds.width(), 1.0), 118.0 / std::max(bounds.height(), 1.0));
    painter.translate(96, 72);
    painter.scale(scale, scale);
    painter.translate(-bounds.center());
    for (const auto &triangle : triangles) {
        painter.setPen(QPen(triangle.color, 0.4 / scale));
        painter.setBrush(triangle.color);
        painter.drawPolygon(triangle.points);
    }
    return result;
}
AssetThumbnails::AssetThumbnails(QObject *parent) : QObject(parent) {
    pool.setMaxThreadCount(2);
    cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/thumbnails-v2";
    QDir().mkpath(cacheDirectory);
}
AssetThumbnails::~AssetThumbnails() {
    pool.waitForDone();
}
QImage AssetThumbnails::request(const Asset &asset) {
    auto recipe = materialPresets().dump() + asset.recipe.dump() +
                  QString("/%1/%2/%3").arg(asset.width).arg(asset.height).arg(asset.depth).toStdString();
    auto key = QString::fromLatin1(
        QCryptographicHash::hash(QByteArray::fromStdString(recipe), QCryptographicHash::Sha256).toHex());
    if (images.contains(key))
        return images.at(key);
    if (pending.contains(key))
        return {};
    auto path = cacheDirectory + "/" + key + ".png";
    QImage cached(path);
    if (cached.size() == QSize(192, 144)) {
        images[key] = cached;
        return cached;
    }
    pending.insert(key);
    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, key, id = asset.id, path] {
        auto image = watcher->result();
        watcher->deleteLater();
        pending.erase(key);
        if (image.isNull())
            return;
        images[key] = image;
        QSaveFile file(path);
        if (file.open(QIODevice::WriteOnly) && image.save(&file, "PNG"))
            file.commit();
        emit ready(id, image);
    });
    watcher->setFuture(QtConcurrent::run(&pool, [asset] {
        try {
            return renderAssetThumbnail(asset);
        } catch (const Standard_Failure &) {
            return QImage{};
        } catch (const std::exception &) {
            return QImage{};
        }
    }));
    return {};
}
} // namespace lmx
