#include "recovery_store.h"
#include "project_store.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <algorithm>
#include <stdexcept>

namespace lmx {
RecoveryStore::RecoveryStore(QString path) : directory(QDir(path).absolutePath()) {
}
QString RecoveryStore::save(const Document &document) const {
    document.validate();
    if (!QDir().mkpath(directory))
        throw std::runtime_error("Não foi possível criar o diretório de recuperação");
    auto prefix = QString::fromStdString(document.id) + "-";
    auto filename = prefix + QString::number(QDateTime::currentMSecsSinceEpoch()) + "-" +
                    QUuid::createUuid().toString(QUuid::WithoutBraces) + ".lmx";
    auto path = directory + "/" + filename;
    ProjectStore::save(path, document, false);
    auto files =
        QDir(directory).entryInfoList({prefix + "*.lmx"}, QDir::Files | QDir::NoSymLinks, QDir::Time);
    for (std::size_t i = 5; i < static_cast<std::size_t>(files.size()); ++i)
        if (!QFile::remove(files[static_cast<qsizetype>(i)].absoluteFilePath()))
            throw std::runtime_error("Autosave salvo, mas a retenção não pôde remover uma versão antiga");
    return path;
}
std::vector<RecoveryEntry> RecoveryStore::entries(QStringList *errors) const {
    std::vector<RecoveryEntry> result;
    auto files = QDir(directory).entryInfoList({"*.lmx"}, QDir::Files | QDir::NoSymLinks, QDir::Time);
    // Bound work at startup even if a third party populates the directory.
    if (files.size() > 100 && errors)
        errors->append("Somente os 100 autosaves mais recentes foram examinados");
    for (qsizetype i = 0; i < std::min<qsizetype>(files.size(), 100); ++i) {
        try {
            auto document = ProjectStore::open(files[i].absoluteFilePath());
            result.push_back({files[i], document.id, document.name});
        } catch (const std::exception &e) {
            if (errors)
                errors->append(files[i].fileName() + ": " + QString::fromUtf8(e.what()));
        }
    }
    return result;
}
void RecoveryStore::clear(const std::string &projectId) const {
    if (QUuid(QString::fromStdString(projectId)).isNull())
        throw std::invalid_argument("Identidade inválida para limpar autosaves");
    for (const auto &file : QDir(directory).entryInfoList({QString::fromStdString(projectId) + "-*.lmx"},
                                                          QDir::Files | QDir::NoSymLinks))
        if (!QFile::remove(file.absoluteFilePath()))
            throw std::runtime_error("Não foi possível remover um autosave");
}
} // namespace lmx
