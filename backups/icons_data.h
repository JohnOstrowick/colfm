#pragma once
// icons_loader.h — parse the embedded icons.xml and (optionally) write PNGs to disk.
//
// Requires QtCore. Usage:
//   #include "icons_loader.h"
//   auto map = IconsData::loadIconsMap();              // name -> PNG bytes
//   IconsData::writeAllToDir(":/tmp/icons_out", nullptr);  // write all to folder (optional)

#include <QByteArray>
#include <QFile>
#include <QMap>
#include <QString>
#include <QXmlStreamReader>
#include <QDir>

#include "icons_data.h"  // provides IconsData::iconsXmlView()

namespace IconsData {

// Parse compiled-in XML into a map: icon name -> PNG data (raw bytes).
inline QMap<QString, QByteArray> loadIconsMap() {
    QMap<QString, QByteArray> out;

    // Convert the embedded blob to a QString for QXmlStreamReader
    const auto view = IconsData::iconsXmlView();
    const QString xml = QString::fromUtf8(view.data(), static_cast<int>(view.size()));

    QXmlStreamReader xr(xml);
    QString curName;
    QString curDataB64;

    while (!xr.atEnd()) {
        xr.readNext();

        if (xr.isStartElement()) {
            const QString tag = xr.name().toString();
            if (tag == QLatin1String("icon")) {
                // Reset per-icon fields
                curName.clear();
                curDataB64.clear();

                // Consume children of <icon>
                while (!(xr.isEndElement() && xr.name() == QLatin1String("icon")) && !xr.atEnd()) {
                    xr.readNext();
                    if (xr.isStartElement()) {
                        const QString sub = xr.name().toString();
                        if (sub == QLatin1String("name")) {
                            curName = xr.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).trimmed();
                        } else if (sub == QLatin1String("data")) {
                            curDataB64 = xr.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement).trimmed();
                        } else {
                            // Skip unknown subelements
                            xr.skipCurrentElement();
                        }
                    }
                }

                if (!curName.isEmpty() && !curDataB64.isEmpty()) {
                    out.insert(curName, QByteArray::fromBase64(curDataB64.toUtf8()));
                }
            }
        }
    }
    return out;
}

// Write all embedded icons to a directory. Returns true on success; false if any write fails.
// If errorOut is provided, the first error message is stored there.
inline bool writeAllToDir(const QString &dirPath, QString *errorOut) {
    const QMap<QString, QByteArray> map = loadIconsMap();
    QDir d;
    if (!d.mkpath(dirPath)) {
        if (errorOut) *errorOut = QStringLiteral("Failed to create directory: %1").arg(dirPath);
        return false;
    }

    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const QString filePath = QDir(dirPath).filePath(it.key());
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly)) {
            if (errorOut) *errorOut = QStringLiteral("Failed to open for write: %1").arg(filePath);
            return false;
        }
        if (f.write(it.value()) != it.value().size()) {
            if (errorOut) *errorOut = QStringLiteral("Short write: %1").arg(filePath);
            return false;
        }
        f.close();
    }
    return true;
}

} // namespace IconsData
