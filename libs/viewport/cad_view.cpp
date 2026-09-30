#include "cad_view.h"
#include "geometry/geometry.h"
#include <AIS_TexturedShape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_GridType.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <OpenGl_GraphicDriver.hxx>
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
    setAccessibleName(tr("Área de projeto CAD"));
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
            if (mat.contains("baseColorTexture")) {
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
            if (!mat.contains("baseColorTexture"))
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
gp_Pnt CadView::position(const QPoint &pixel) const {
    double x, y, z, vx, vy, vz;
    view->ConvertWithProj(qRound(pixel.x() * devicePixelRatioF()), qRound(pixel.y() * devicePixelRatioF()), x,
                          y, z, vx, vy, vz);
    if (std::abs(vz) < 1e-8)
        return gp_Pnt(0, 0, 0);
    x -= z * vx / vz;
    y -= z * vy / vz;
    if (snap) {
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
    if (tool != "select")
        setTop(true);
    setCursor(tool == "select" ? Qt::ArrowCursor : Qt::CrossCursor);
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
    }
}
void CadView::mouseMoveEvent(QMouseEvent *e) {
    if (view.IsNull())
        return;
    auto p = position(e->pos());
    emit coordinates(tr("X %1  Y %2 mm%3")
                         .arg(p.X(), 0, 'f', 1)
                         .arg(p.Y(), 0, 'f', 1)
                         .arg(snap ? tr(" · SNAP") : QString{}));
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
void CadView::mouseReleaseEvent(QMouseEvent *) {
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
        auto p = position(e->position().toPoint());
        auto object = Library::instantiate(*a, p.X(), p.Y());
        clearPreview();
        preview = new AIS_Shape(compound(buildEntity(current, object)));
        preview->SetColor(Quantity_NOC_CYAN1);
        preview->SetTransparency(0.6);
        context->Display(preview, false);
        context->Deactivate(preview);
        view->Redraw();
        e->acceptProposedAction();
    } catch (const std::exception &error) {
        emit failure(QString::fromUtf8(error.what()));
    }
}
void CadView::dragLeaveEvent(QDragLeaveEvent *) {
    clearPreview();
}
void CadView::dropEvent(QDropEvent *e) {
    if (view.IsNull())
        return;
    auto p = position(e->position().toPoint());
    clearPreview();
    emit assetDropped(QString::fromUtf8(e->mimeData()->data("application/x-libremax-asset")), p.X(), p.Y());
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
