#pragma once
#include "library/library.h"
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
namespace lmx {
class ModelImporter final : public QObject {
    Q_OBJECT
    QProcess process;
    QTemporaryDir temporary;
    QTimer timeout;
    QString source;
    QByteArray log;
    bool complete = false;
    void fail(const QString &message);

  public:
    explicit ModelImporter(QObject *parent = nullptr);
    void start(const QString &file, const QString &blender, const QString &script);
    void cancel();
  signals:
    void ready(const lmx::Asset &asset, const lmx::Json &details);
    void failed(const QString &message);
};
void storeUserAsset(Library &library, const QString &directory, const Asset &asset);
} // namespace lmx
