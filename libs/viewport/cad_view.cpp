#include "cad_view.h"
#include "geometry/geometry.h"
#include <AIS_TexturedShape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_GridType.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Bnd_Box.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QToolTip>
#include <QWheelEvent>
#include <Standard_Failure.hxx>
#include <V3d_Viewer.hxx>
#include <cmath>
#include <numbers>
#include <set>
#ifdef Q_OS_WIN
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif

namespace lmx {
CadView::CadView(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setMinimumSize(320, 240);
    setObjectName("cadView");
    setAccessibleName(tr("Seu apartamento: arraste móveis, clique para selecionar"));
}
void CadView::initialize() {
    if (!view.IsNull())
        return;
    try {
        Handle(Aspect_DisplayConnection) display = new Aspect_DisplayConnection;
        Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(display);
        Handle(V3d_Viewer) viewer = new V3d_Viewer(driver);
        viewer->SetDefaultLights();
        viewer->SetLightOn();
        context = new AIS_InteractiveContext(viewer);
        context->SetDisplayMode(AIS_Shaded, false);
        view = viewer->CreateView();
#ifdef Q_OS_WIN
        Handle(WNT_Window) window = new WNT_Window(reinterpret_cast<Aspect_Handle>(winId()));
#else
        Handle(Xw_Window) window = new Xw_Window(display, static_cast<Aspect_Drawable>(winId()));
#endif
        view->SetWindow(window);
        if (!window->IsMapped())
            window->Map();
        view->SetBackgroundColor(Quantity_Color(0.008, 0.014, 0.022, Quantity_TOC_RGB));
        view->ChangeRenderingParams().NbMsaaSamples = 4;
        view->SetProj(V3d_Zpos);
        view->SetScale(6000);
        viewer->SetRectangularGridValues(0, 0, 100, 100, 0);
        viewer->SetRectangularGridGraphicValues(10000, 10000, 0);
        viewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
        view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.08, V3d_ZBUFFER);
        view->MustBeResized();
        scene(current);
    } catch (const Standard_Failure &e) {
        emit failure(QString::fromUtf8(e.GetMessageString()));
    }
}
void CadView::paintEvent(QPaintEvent *) {
    initialize();
    if (!view.IsNull())
        view->Redraw();
}
void CadView::resizeEvent(QResizeEvent *) {
    if (!view.IsNull())
        view->MustBeResized();
}
void CadView::scene(const Document &d) {
    current = d;
    movingObject.reset();
    pendingPlacement.reset();
    if (context.IsNull())
        return;
    try {
        context->RemoveAll(false);
        preview.Nullify();
        owners.clear();
        for (const auto &part : buildScene(d)) {
            const auto &owner = d.at(part.owner);
            if (cutaway && owner.type == "Ceiling")
                continue;
            const Entity *wall = &owner;
            if (owner.type == "Door" || owner.type == "Window")
                wall = &d.at(owner.parent);
            if (cutaway && !top && (wall->type == "Wall" || wall->type == "HalfWall")) {
                double angle = wall->transform.yaw * std::numbers::pi / 180;
                if (std::sin(angle) - std::cos(angle) > 0.01)
                    continue;
            }
            Handle(AIS_Shape) shape = new AIS_Shape(part.shape);
            Json mat;
            for (const auto &m : d.materials)
                if (m.at("id") == part.material) {
                    mat = m;
                    break;
                }
            if (mat.is_null())
                throw std::invalid_argument("Material de componente não encontrado");
            if (mat.contains("baseColorTexture") && owner.type != "MeshObject") {
                auto hash = mat.at("baseColorTexture").get<std::string>();
                auto filename = textureCache.filePath(QString::fromStdString(hash) + ".png");
                if (!QFile::exists(filename)) {
                    QFile textureFile(filename);
                    if (!textureFile.open(QIODevice::WriteOnly) ||
                        textureFile.write(d.embeddedAssets.at(hash)) < 0)
                        throw std::runtime_error("Falha ao preparar textura de viewport");
                }
                Handle(AIS_TexturedShape) textured = new AIS_TexturedShape(part.shape);
                textured->SetTextureFileName(QFile::encodeName(filename).constData());
                textured->SetTextureMapOn();
                textured->SetTextureScale(true, mat.value("textureScale", 1000.0),
                                          mat.value("textureScale", 1000.0));
                textured->SetDisplayMode(3);
                shape = textured;
            }
            auto c = mat.at("baseColor");
            shape->SetColor(
                Quantity_Color(c[0].get<double>(), c[1].get<double>(), c[2].get<double>(), Quantity_TOC_RGB));
            if (mat.value("transmission", 0.0) > 0)
                shape->SetTransparency(0.65);
            if (owner.metadata.value("format", std::string{}) == "DXF") {
                shape->SetColor(Quantity_NOC_CYAN1);
                shape->SetWidth(1.5);
            }
            if (!mat.contains("baseColorTexture") || owner.type == "MeshObject")
                shape->SetDisplayMode(AIS_Shaded);
            context->Display(shape, false);
            if (d.at(part.owner).locked)
                context->Deactivate(shape);
            owners.emplace(shape.get(), part.owner);
        }
        view->Redraw();
    } catch (const Standard_Failure &e) {
        emit failure(QString::fromUtf8(e.GetMessageString()));
    } catch (const std::exception &e) {
        emit failure(QString::fromUtf8(e.what()));
    }
}
gp_Pnt CadView::position(const QPoint &pixel, bool applySnap) const {
    double x, y, z, vx, vy, vz;
    view->ConvertWithProj(qRound(pixel.x() * devicePixelRatioF()), qRound(pixel.y() * devicePixelRatioF()), x,
                          y, z, vx, vy, vz);
    if (std::abs(vz) < 1e-8)
        return gp_Pnt(0, 0, 0);
    x -= z * vx / vz;
    y -= z * vy / vz;
    if (snap && applySnap) {
        gp_Pnt raw(x, y, 0), nearest;
        double best = std::max(40.0, view->Scale() / width() * 10);
        for (const auto &e : current.entities)
            if (e.type == "Wall" || e.type == "HalfWall") {
                double a = e.transform.yaw * std::numbers::pi / 180;
                for (const auto &p : {gp_Pnt(e.transform.x, e.transform.y, 0),
                                      gp_Pnt(e.transform.x + e.width * std::cos(a),
                                             e.transform.y + e.width * std::sin(a), 0)})
                    if (raw.Distance(p) < best) {
                        best = raw.Distance(p);
                        nearest = p;
                    }
            }
        if (best < std::max(40.0, view->Scale() / width() * 10))
            return nearest;
        x = std::round(x / grid) * grid;
        y = std::round(y / grid) * grid;
    }
    return gp_Pnt(millimeters(x), millimeters(y), 0);
}
void CadView::clearPreview() {
    if (!preview.IsNull() && !context.IsNull()) {
        context->Remove(preview, false);
        preview.Nullify();
        view->Redraw();
    }
}
void CadView::setTool(const QString &mode) {
    tool = mode;
    wallStart.reset();
    clearPreview();
    placingAsset.reset();
    movingObject.reset();
    pendingPlacement.reset();
    emit placementStatus({}, false, true);
    if (tool != "select")
        setTop(true);
    setCursor(tool == "select" ? Qt::ArrowCursor : Qt::CrossCursor);
}
void CadView::beginPlacement(const QString &id) {
    if (!findAsset)
        return;
    setTool("select");
    placingAsset = findAsset(id);
    placementYaw = 0;
    if (placingAsset) {
        setCursor(Qt::CrossCursor);
        emit placementStatus(
            tr("%1 · mova o mouse, clique para colocar · R gira · Esc cancela").arg(placingAsset->name), true,
            true);
    }
}
std::pair<gp_Pnt, std::string> CadView::surfacePosition(const QPoint &pixel) {
    auto floor = position(pixel, false);
    if (top)
        return {floor, {}};
    context->MoveTo(qRound(pixel.x() * devicePixelRatioF()), qRound(pixel.y() * devicePixelRatioF()), view,
                    false);
    if (!context->HasDetected())
        return {floor, {}};
    auto found = owners.find(context->DetectedInteractive().get());
    if (found == owners.end())
        return {floor, {}};
    const auto &wall = current.at(found->second);
    if (movable(wall)) {
        double x, y, z, vx, vy, vz;
        view->ConvertWithProj(qRound(pixel.x() * devicePixelRatioF()),
                              qRound(pixel.y() * devicePixelRatioF()), x, y, z, vx, vy, vz);
        if (std::abs(vz) > 1e-8) {
            const double t = (wall.transform.z + wall.height - z) / vz;
            return {gp_Pnt(x + t * vx, y + t * vy, 0), {}};
        }
    }
    if (wall.type != "Wall" && wall.type != "HalfWall")
        return {floor, {}};
    double x, y, z, vx, vy, vz;
    view->ConvertWithProj(qRound(pixel.x() * devicePixelRatioF()), qRound(pixel.y() * devicePixelRatioF()), x,
                          y, z, vx, vy, vz);
    const double angle = wall.transform.yaw * std::numbers::pi / 180, nx = -std::sin(angle),
                 ny = std::cos(angle);
    const double denominator = vx * nx + vy * ny;
    if (std::abs(denominator) < 1e-8)
        return {floor, {}};
    const double distance = (x - wall.transform.x) * nx + (y - wall.transform.y) * ny;
    const double side = distance >= 0 ? 1 : -1;
    const double t = (side * wall.depth / 2 - distance) / denominator;
    return {gp_Pnt(x + t * vx, y + t * vy, 0), wall.id};
}
void CadView::showPlacement(Placement placement, const Asset *asset) {
    clearPreview();
    pendingPlacement = placement;
    auto d = current;
    if (asset)
        Library::attachModel(d, *asset);
    if ((placement.object.type == "Door" || placement.object.type == "Window") &&
        placement.object.parent.empty()) {
        emit placementStatus(QString::fromStdString(placement.message), true, false);
        return;
    }
    preview = new AIS_Shape(compound(buildEntity(d, placement.object)));
    preview->SetColor(placement.allowed ? Quantity_Color(0.25, 0.85, 0.63, Quantity_TOC_RGB)
                                        : Quantity_Color(1, 0.22, 0.16, Quantity_TOC_RGB));
    preview->SetTransparency(0.35);
    context->Display(preview, false);
    context->Deactivate(preview);
    view->Redraw();
    emit placementStatus(QString::fromStdString(placement.message) +
                             (placingAsset ? tr(" · R gira · Esc cancela") : tr(" · Esc cancela")),
                         true, placement.allowed);
}
void CadView::previewAsset(const Asset &asset, const QPoint &pixel) {
    const auto [p, wall] = surfacePosition(pixel);
    auto object = Library::instantiate(asset, 0, 0);
    object.transform.yaw = placementYaw;
    auto placement = placeObject(current, object, p.X(), p.Y(), assist, wall);
    showPlacement(placement, &asset);
}
void CadView::setTop(bool enabled) {
    top = enabled;
    wallStart.reset();
    clearPreview();
    if (view.IsNull())
        return;
    view->SetProj(enabled ? V3d_Zpos : V3d_XposYposZpos);
    if (enabled)
        view->Viewer()->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
    else
        view->Viewer()->DeactivateGrid();
    scene(current);
    view->Redraw();
}
void CadView::setGrid(double step) {
    grid = step;
    if (!view.IsNull()) {
        view->Viewer()->SetRectangularGridValues(0, 0, grid, grid, 0);
        view->Redraw();
    }
}
void CadView::frame() {
    if (!view.IsNull()) {
        view->FitAll(0.12, false);
        view->ZFitAll();
        view->Redraw();
    }
}
void CadView::frameRoom(const std::string &id) {
    if (view.IsNull() || !current.contains(id))
        return;
    const auto &room = current.at(id);
    Bnd_Box box;
    box.Add(gp_Pnt(room.transform.x - 200, room.transform.y - 200, 0));
    box.Add(gp_Pnt(room.transform.x + room.width + 200, room.transform.y + room.depth + 200,
                   top ? 0 : room.height));
    view->FitAll(box, 0.08, false);
    view->ZFitAll();
    view->Redraw();
}
void CadView::selection() {
    std::set<std::string> ids;
    for (context->InitSelected(); context->MoreSelected(); context->NextSelected()) {
        auto it = owners.find(context->SelectedInteractive().get());
        if (it != owners.end())
            ids.insert(it->second);
    }
    QStringList result;
    for (const auto &id : ids)
        result << QString::fromStdString(id);
    emit selected(result);
}
void CadView::select(const std::vector<std::string> &ids) {
    if (context.IsNull())
        return;
    context->ClearSelected(false);
    for (const auto &[object, id] : owners)
        if (std::find(ids.begin(), ids.end(), id) != ids.end()) {
            Handle(AIS_InteractiveObject) handle = const_cast<AIS_InteractiveObject *>(object);
            context->AddOrRemoveSelected(handle, false);
        }
    view->Redraw();
}
void CadView::mousePressEvent(QMouseEvent *e) {
    if (view.IsNull())
        return;
    setFocus();
    last = e->pos();
    pressed = e->pos();
    moved = false;
    if (e->button() == Qt::LeftButton && placingAsset) {
        previewAsset(*placingAsset, e->pos());
        if (pendingPlacement && pendingPlacement->allowed) {
            auto object = pendingPlacement->object;
            auto id = placingAsset->id;
            setTool("select");
            emit assetDropped(id, object);
        }
        return;
    }
    if (e->button() == Qt::RightButton && tool != "select") {
        wallStart.reset();
        clearPreview();
        return;
    }
    if (e->button() == Qt::LeftButton && (tool == "wall" || tool == "half")) {
        auto p = position(e->pos());
        if (!wallStart) {
            wallStart = p;
            return;
        }
        if (p.Distance(*wallStart) < 1)
            return;
        auto start = *wallStart;
        wallStart = p;
        clearPreview();
        emit wallCreated(start.X(), start.Y(), p.X(), p.Y(), tool == "half");
        return;
    }
    if (e->button() == Qt::RightButton && !top)
        view->StartRotation(e->pos().x(), e->pos().y());
    if (e->button() == Qt::LeftButton && tool == "select") {
        context->MoveTo(qRound(e->pos().x() * devicePixelRatioF()),
                        qRound(e->pos().y() * devicePixelRatioF()), view, false);
        context->SelectDetected(e->modifiers() & Qt::ControlModifier ? AIS_SelectionScheme_Add
                                                                     : AIS_SelectionScheme_Replace);
        view->Redraw();
        selection();
        movingObject.reset();
        if (!(e->modifiers() & Qt::ControlModifier) && context->HasDetected()) {
            auto owner = owners.find(context->DetectedInteractive().get());
            if (owner != owners.end() && movable(current.at(owner->second))) {
                movingObject = current.at(owner->second);
                const auto point = position(e->pos(), false);
                const double a = movingObject->transform.yaw * std::numbers::pi / 180;
                grabOffset = {
                    movingObject->transform.x +
                        (movingObject->width * std::cos(a) - movingObject->depth * std::sin(a)) / 2 -
                        point.X(),
                    movingObject->transform.y +
                        (movingObject->width * std::sin(a) + movingObject->depth * std::cos(a)) / 2 -
                        point.Y()};
            }
        }
    }
}
void CadView::mouseMoveEvent(QMouseEvent *e) {
    if (view.IsNull())
        return;
    auto p = position(e->pos());
    if (placingAsset) {
        previewAsset(*placingAsset, e->pos());
        return;
    }
    if (movingObject && (e->buttons() & Qt::LeftButton) &&
        (moved || (e->pos() - pressed).manhattanLength() >= QApplication::startDragDistance())) {
        moved = true;
        auto raw = position(e->pos(), false);
        auto [surface, wall] = surfacePosition(e->pos());
        auto placement =
            placeObject(current, *movingObject, wall.empty() ? raw.X() + grabOffset.x() : surface.X(),
                        wall.empty() ? raw.Y() + grabOffset.y() : surface.Y(), assist, wall);
        showPlacement(placement);
        return;
    }
    emit coordinates(tr("X %1  Y %2 mm%3")
                         .arg(p.X(), 0, 'f', 1)
                         .arg(p.Y(), 0, 'f', 1)
                         .arg(snap ? tr(" · alinhamento ligado") : QString{}));
    if (e->buttons() & Qt::MiddleButton) {
        auto delta = e->pos() - last;
        view->Pan(delta.x(), -delta.y());
        last = e->pos();
        return;
    }
    if (e->buttons() & Qt::RightButton && !top && tool == "select") {
        view->Rotation(e->pos().x(), e->pos().y());
        return;
    }
    if (wallStart) {
        clearPreview();
        if (p.Distance(*wallStart) > 0.1) {
            preview = new AIS_Shape(BRepBuilderAPI_MakeEdge(*wallStart, p).Shape());
            preview->SetColor(Quantity_NOC_CYAN1);
            context->Display(preview, false);
            context->Deactivate(preview);
            QToolTip::showText(e->globalPosition().toPoint(),
                               tr("%1 mm · clique para continuar").arg(p.Distance(*wallStart), 0, 'f', 1),
                               this);
            view->Redraw();
        }
    } else {
        context->MoveTo(qRound(e->pos().x() * devicePixelRatioF()),
                        qRound(e->pos().y() * devicePixelRatioF()), view, true);
    }
}
void CadView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || !movingObject)
        return;
    auto pending = pendingPlacement;
    const bool commit = moved && pending && pending->allowed;
    movingObject.reset();
    pendingPlacement.reset();
    moved = false;
    clearPreview();
    emit placementStatus({}, false, true);
    if (commit)
        emit objectMoved(pending->object);
}
void CadView::wheelEvent(QWheelEvent *e) {
    if (view.IsNull())
        return;
    view->StartZoomAtPoint(qRound(e->position().x() * devicePixelRatioF()),
                           qRound(e->position().y() * devicePixelRatioF()));
    view->ZoomAtPoint(0, 0, 0, e->angleDelta().y() / 4);
    e->accept();
}
void CadView::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        setTool("select");
        e->accept();
    } else if (e->key() == Qt::Key_R && placingAsset) {
        placementYaw = std::fmod(placementYaw + 90, 360);
        previewAsset(*placingAsset, mapFromGlobal(QCursor::pos()));
        e->accept();
    } else
        QWidget::keyPressEvent(e);
}
void CadView::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasFormat("application/x-libremax-asset"))
        e->acceptProposedAction();
}
void CadView::dragMoveEvent(QDragMoveEvent *e) {
    if (view.IsNull() || !findAsset)
        return;
    try {
        auto a = findAsset(QString::fromUtf8(e->mimeData()->data("application/x-libremax-asset")));
        if (!a)
            return;
        previewAsset(*a, e->position().toPoint());
        e->acceptProposedAction();
    } catch (const std::exception &error) {
        emit failure(QString::fromUtf8(error.what()));
    }
}
void CadView::dragLeaveEvent(QDragLeaveEvent *) {
    clearPreview();
    pendingPlacement.reset();
    emit placementStatus({}, false, true);
}
void CadView::dropEvent(QDropEvent *e) {
    if (view.IsNull())
        return;
    auto id = QString::fromUtf8(e->mimeData()->data("application/x-libremax-asset"));
    if (!findAsset)
        return;
    auto asset = findAsset(id);
    if (!asset)
        return;
    previewAsset(*asset, e->position().toPoint());
    auto pending = pendingPlacement;
    clearPreview();
    pendingPlacement.reset();
    if (!pending || !pending->allowed) {
        e->ignore();
        return;
    }
    emit placementStatus({}, false, true);
    emit assetDropped(id, pending->object);
    e->acceptProposedAction();
}
void CadView::capture(const QString &path) {
    if (view.IsNull() || !view->Dump(QFile::encodeName(path).constData()))
        throw std::runtime_error("Não foi possível capturar viewport");
}
QPoint CadView::project(double x, double y, double z) const {
    int px = 0, py = 0;
    if (!view.IsNull())
        view->Convert(x, y, z, px, py);
    return QPoint(qRound(px / devicePixelRatioF()), qRound(py / devicePixelRatioF()));
}
} // namespace lmx
