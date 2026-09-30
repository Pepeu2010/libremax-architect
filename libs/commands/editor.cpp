#include "editor.h"
#include <QUndoCommand>
namespace lmx {
namespace {
class Change final : public QUndoCommand {
    Editor &editor;
    Document before, after;

  public:
    Change(Editor &e, Document b, Document a, const QString &name)
        : QUndoCommand(name), editor(e), before(std::move(b)), after(std::move(a)) {}
    void undo() override { editor.restore(before); }
    void redo() override { editor.restore(after); }
};
} // namespace
Editor::Editor(QObject *parent) : QObject(parent) {
    history.setUndoLimit(200);
}
void Editor::restore(const Document &d) {
    document_ = d;
    emit changed();
}
void Editor::load(const Document &d) {
    d.validate();
    history.clear();
    document_ = d;
    history.setClean();
    emit changed();
}
void Editor::apply(const QString &name, const std::function<void(Document &)> &change) {
    Document next = document_;
    change(next);
    next.validate();
    if (next.serialize() != document_.serialize())
        history.push(new Change(*this, document_, std::move(next), name));
}
} // namespace lmx
