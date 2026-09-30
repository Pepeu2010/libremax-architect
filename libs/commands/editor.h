#pragma once
#include "document/document.h"
#include <QObject>
#include <QUndoStack>
#include <functional>
namespace lmx {
class Editor final : public QObject {
    Q_OBJECT
    Document document_;

  public:
    QUndoStack history;
    explicit Editor(QObject *parent = nullptr);
    const Document &document() const { return document_; }
    void restore(const Document &d);
    void load(const Document &d);
    void apply(const QString &name, const std::function<void(Document &)> &change);
  signals:
    void changed();
};
} // namespace lmx
