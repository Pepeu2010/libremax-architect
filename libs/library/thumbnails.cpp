#include "thumbnails.h"
#include "geometry/geometry.h"
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <QCryptographicHash>
#include <QDir>
#include <QFutureWatcher>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtConcurrent>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace lmx {
QImage renderAssetThumbnail(const Asset &asset) {
    Document document;
    auto object = Library::instantiate(asset, 0, 0);
    if (object.type == "Door" || object.type == "Window") {
        auto supporting = wall(0, 0, 5000, 0);
        object.parent = supporting.id;
        document.entities.push_back(supporting);
    }
    Library::attachModel(document, asset);
    document.entities.push_back(object);
    document.validate();
    struct Triangle {
        std::array<QPointF, 3> points;
        std::array<QColor, 3> colors;
        std::array<double, 3> depths;
    };
    std::vector<Triangle> triangles;
    QRectF bounds;
    bool first = true;
    for (const auto &part : buildEntity(document, object)) {
        if (object.type != "MeshObject") {
            BRepMesh_IncrementalMesh mesh(part.shape, 3.0, false, 0.5, false);
            if (!mesh.IsDone())
                throw std::runtime_error("Falha ao gerar miniatura");
        }
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
                const auto &color = material->at("baseColor");
                Triangle triangle;
                const std::array<int, 3> nodes{a, b, c};
                const std::array<gp_Pnt, 3> points{p, q, r};
                for (std::size_t vertex = 0; vertex < points.size(); ++vertex) {
                    const auto &point = points[vertex];
                    auto shadingNormal = normal;
                    if (meshData->HasNormals()) {
                        shadingNormal = gp_Vec(meshData->Normal(nodes[vertex]));
                        shadingNormal.Transform(location.Transformation());
                        if (face.Orientation() == TopAbs_REVERSED)
                            shadingNormal.Reverse();
                    }
                    double shade = 0.55;
                    if (shadingNormal.Magnitude() > 1e-8)
                        shade +=
                            0.45 *
                            std::max(0.0, shadingNormal.Normalized().Dot(gp_Vec(-0.4, 0.5, 1).Normalized()));
                    triangle.colors[vertex].setRgbF(std::pow(color[0].get<double>() * shade, 1 / 2.2),
                                                    std::pow(color[1].get<double>() * shade, 1 / 2.2),
                                                    std::pow(color[2].get<double>() * shade, 1 / 2.2));
                    QPointF projected((point.X() + point.Y()) * 0.82,
                                      (point.Y() - point.X()) * 0.36 - point.Z());
                    triangle.points[vertex] = projected;
                    // Depth axis is orthogonal to both orthographic projection axes.
                    triangle.depths[vertex] = point.Y() - point.X() + 0.72 * point.Z();
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
                triangles.push_back(triangle);
            }
        }
    }
    if (triangles.empty())
        throw std::runtime_error("Miniatura sem geometria");
    // A depth buffer resolves intersecting triangles and the backs of broad surfaces.
    // Two-times supersampling keeps curved edges clean without a GPU context per worker.
    constexpr int width = 384, height = 288;
    QImage result(width, height, QImage::Format_RGB32);
    result.fill(QColor("#17171f"));
    std::vector<double> depths(width * height, -std::numeric_limits<double>::infinity());
    const double scale =
        std::min(332.0 / std::max(bounds.width(), 1.0), 236.0 / std::max(bounds.height(), 1.0));
    for (auto &triangle : triangles) {
        for (auto &point : triangle.points)
            point = (point - bounds.center()) * scale + QPointF(width / 2, height / 2);
        const auto &a = triangle.points[0], &b = triangle.points[1], &c = triangle.points[2];
        const double denominator = (b.y() - c.y()) * (a.x() - c.x()) + (c.x() - b.x()) * (a.y() - c.y());
        if (std::abs(denominator) < 1e-8)
            continue;
        const int left = std::max(0, static_cast<int>(std::floor(std::min({a.x(), b.x(), c.x()}))));
        const int right = std::min(width - 1, static_cast<int>(std::ceil(std::max({a.x(), b.x(), c.x()}))));
        const int top = std::max(0, static_cast<int>(std::floor(std::min({a.y(), b.y(), c.y()}))));
        const int bottom = std::min(height - 1, static_cast<int>(std::ceil(std::max({a.y(), b.y(), c.y()}))));
        for (int y = top; y <= bottom; ++y) {
            auto *pixels = reinterpret_cast<QRgb *>(result.scanLine(y));
            for (int x = left; x <= right; ++x) {
                const double u =
                    ((b.y() - c.y()) * (x + .5 - c.x()) + (c.x() - b.x()) * (y + .5 - c.y())) / denominator;
                const double v =
                    ((c.y() - a.y()) * (x + .5 - c.x()) + (a.x() - c.x()) * (y + .5 - c.y())) / denominator;
                const double w = 1 - u - v;
                if (u < -1e-8 || v < -1e-8 || w < -1e-8)
                    continue;
                const double depth = u * triangle.depths[0] + v * triangle.depths[1] + w * triangle.depths[2];
                auto &previous = depths[y * width + x];
                if (depth <= previous)
                    continue;
                previous = depth;
                const auto &first = triangle.colors[0], &second = triangle.colors[1],
                           &third = triangle.colors[2];
                pixels[x] = qRgb(static_cast<int>(u * first.red() + v * second.red() + w * third.red()),
                                 static_cast<int>(u * first.green() + v * second.green() + w * third.green()),
                                 static_cast<int>(u * first.blue() + v * second.blue() + w * third.blue()));
            }
        }
    }
    return result.scaled(192, 144, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
AssetThumbnails::AssetThumbnails(QObject *parent) : QObject(parent) {
    pool.setMaxThreadCount(2);
    cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/thumbnails-v4";
    QDir().mkpath(cacheDirectory);
}
AssetThumbnails::~AssetThumbnails() {
    pool.waitForDone();
}
void AssetThumbnails::setWorkerLimit(int workers) {
    pool.setMaxThreadCount(std::clamp(workers, 1, 4));
}
QImage AssetThumbnails::request(const Asset &asset, std::function<Asset(const Asset &)> loader) {
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
    watcher->setFuture(QtConcurrent::run(&pool, [asset, loader] {
        try {
            return renderAssetThumbnail(loader ? loader(asset) : asset);
        } catch (const Standard_Failure &) {
            return QImage{};
        } catch (const std::exception &) {
            return QImage{};
        }
    }));
    return {};
}
} // namespace lmx
