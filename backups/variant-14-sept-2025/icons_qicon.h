#pragma once
// icons_qicon.h — build QIcon objects from the compiled-in icons.xml blob.

#include <QIcon>
#include <QMap>
#include <QPixmap>
#include <QString>
#include "icons_loader.h"   // loadIconsMap()

namespace IconsData {

// Lazy, cached bytes map (built once from embedded XML)
inline const QMap<QString,QByteArray>& bytesMap() {
    static const QMap<QString,QByteArray> k = loadIconsMap();
    return k;
}

// Get a QIcon by PNG filename (e.g. "glue.png"). Returns null icon if missing.
inline QIcon getIcon(const QString& name) {
    const auto& m = bytesMap();
    const auto it = m.constFind(name);
    if (it == m.cend()) return QIcon();
    QPixmap px;
    px.loadFromData(*it, "PNG");
    return QIcon(px);
}

} // namespace IconsData
