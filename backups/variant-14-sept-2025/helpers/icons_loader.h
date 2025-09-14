// helpers/icons_loader.h
#pragma once
// Parse embedded icons.xml into name->PNG bytes

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QXmlStreamReader>
#include <QDir>
#include <QFile>

#include "icons_data.h"  // provides IconsData::iconsXmlView()

namespace IconsData {

inline QMap<QString, QByteArray> loadIconsMap() {
    QMap<QString, QByteArray> out;
    const auto view = IconsData::iconsXmlView();
    const QString xml = QString::fromUtf8(view.data(), static_cast<int>(view.size()));

    QXmlStreamReader xr(xml);
    QString curName, curDataB64;

    while (!xr.atEnd()) {
        xr.readNext();
        if (xr.isStartElement() && xr.name() == QLatin1String("icon")) {
            curName.clear(); curDataB64.clear();
            while (!(xr.isEndElement() && xr.name() == QLatin1String("icon")) && !xr.atEnd()) {
                xr.readNext();
                if (xr.isStartElement()) {
                    const auto tag = xr.name();
                    if (tag == QLatin1String("name"))  curName   = xr.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).trimmed();
                    else if (tag == QLatin1String("data")) curDataB64 = xr.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).trimmed();
                    else xr.skipCurrentElement();
                }
            }
            if (!curName.isEmpty() && !curDataB64.isEmpty())
                out.insert(curName, QByteArray::fromBase64(curDataB64.toUtf8()));
        }
    }
    return out;
}

// Optional: write all to a directory
inline bool writeAllToDir(const QString &dirPath, QString *errorOut=nullptr) {
    const auto map = loadIconsMap();
    QDir d; if (!d.mkpath(dirPath)) { if (errorOut) *errorOut = u"mkpath failed: %1"_qs.arg(dirPath); return false; }
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        QFile f(QDir(dirPath).filePath(it.key()));
        if (!f.open(QIODevice::WriteOnly)) { if (errorOut) *errorOut = u"open failed: %1"_qs.arg(f.fileName()); return false; }
        if (f.write(it.value()) != it.value().size()) { if (errorOut) *errorOut = u"short write: %1"_qs.arg(f.fileName()); return false; }
    }
    return true;
}

} // namespace IconsData
