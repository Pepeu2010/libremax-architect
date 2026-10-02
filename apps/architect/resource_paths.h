#pragma once
#include <QCoreApplication>
#include <QFileInfo>
#include <QString>

namespace lmx {
inline QString resourcePath(const QString &relative) {
    const auto installed = QCoreApplication::applicationDirPath() + "/../share/libremax/" + relative;
    if (QFileInfo::exists(installed))
        return installed;
#ifndef LMX_INSTALLED_ONLY
    return QStringLiteral(LMX_SOURCE_DIR) + "/" + relative;
#else
    return installed;
#endif
}
} // namespace lmx
