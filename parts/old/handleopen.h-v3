// handleopen.h
#pragma once
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QMimeDatabase>
#include <QImageReader>
#include <QPixmap>
#include <QDateTime>
#include <QItemSelectionModel>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

// ---- ColFM open/preview helpers ----

inline QModelIndex ColFM::currentIndex() const {
    if (!currentView) return QModelIndex();
    auto *sel = currentView->selectionModel();
    if (!sel) return QModelIndex();
    QModelIndex idx = sel->currentIndex();
    if (!idx.isValid()) {
        const auto rows = sel->selectedRows();
        if (!rows.isEmpty()) return rows.first();
    }
    return idx;
}

inline QString humanSize(qint64 bytes) {
    const char *units[] = {"B","KB","MB","GB","TB"};
    double sz = (double)bytes;
    int u = 0;
    while (sz >= 1024.0 && u < 4) { sz /= 1024.0; ++u; }
    return QString::number(sz, 'f', (u==0?0:1)) + " " + units[u];
}

inline QString permsToString(QFile::Permissions p) {
    auto bit = [p](QFile::Permission perm, QChar c){ return (p & perm) ? c : QChar('-'); };
    return QString() +
        bit(QFile::ReadOwner,  'r') + bit(QFile::WriteOwner, 'w') + bit(QFile::ExeOwner,  'x') +
        bit(QFile::ReadGroup,  'r') + bit(QFile::WriteGroup, 'w') + bit(QFile::ExeGroup,  'x') +
        bit(QFile::ReadOther,  'r') + bit(QFile::WriteOther, 'w') + bit(QFile::ExeOther,  'x');
}

inline bool ColFM::isImageFile(const QString &path) const {
    QMimeDatabase db;
    auto mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    return mt.name().startsWith("image/");
}

inline QString fileInfoHtml(const QFileInfo &fi, const QMimeType &mt, const QString &extra = QString()) {
    const QString name = fi.fileName().toHtmlEscaped();
    const QString type = (mt.isValid() ? mt.name() : "unknown").toHtmlEscaped();
    const QString size = fi.isDir() ? "-" : humanSize(fi.size());
    const QString mod  = fi.lastModified().toString(Qt::ISODate);
    const QString perms = permsToString(fi.permissions());
    const QString owner = fi.owner().toHtmlEscaped();
    const QString group = fi.group().toHtmlEscaped();
    const QString path  = fi.absoluteFilePath().toHtmlEscaped();

    QString html;
    html += "<div style='font-size:12px'>";
    html += "<table style='border-collapse:collapse' cellspacing='0' cellpadding='2'>";
    auto row = [&](const char *k, const QString &v){
        html += QString("<tr><td style='font-weight:bold; padding-right:10px; white-space:nowrap'>%1</td>"
                        "<td style='width:100%%'>%2</td></tr>").arg(k).arg(v);
    };
    row("Name", name);
    row("Kind", type);
    row("Size", size);
    row("Modified", mod);
    row("Permissions", perms);
    row("Owner", owner);
    row("Group", group);
    row("Path", "<tt>" + path + "</tt>");
    if (!extra.isEmpty()) {
        row("Preview", extra);
    }
    html += "</table></div>";
    return html;
}

inline void ColFM::previewFile(const QModelIndex &idx) {
    if (!idx.isValid()) return;
    const QString path = model->filePath(idx);
    QFileInfo fi(path);
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);

    // Build visual (image) if possible
    QPixmap pm;
    if (fi.isFile() && mt.name().startsWith("image/")) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QImage img = reader.read();
        if (!img.isNull()) {
            pm = QPixmap::fromImage(img);
            if (pm.width() > 512) pm = pm.scaledToWidth(512, Qt::SmoothTransformation);
        }
    }

    // Text snippet
    QString textSnippet;
    if (fi.isFile() && mt.name().startsWith("text/") && fi.size() <= 256*1024) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            QString sample = ts.read(4096);
            sample.replace('<', "&lt;").replace('>', "&gt;");
            textSnippet = "<pre style='white-space:pre-wrap; margin:0'>" + sample + "</pre>";
        }
    }

    // For audio/video/office docs, we don’t render inline; show richer info
    if (textSnippet.isEmpty() && pm.isNull()) {
        if (mt.name().startsWith("audio/") || mt.name().startsWith("video/") ||
            mt.name().contains("officedocument")) {
            textSnippet = "(No inline preview. Use Open to launch in the default application.)";
        }
    }

    const QString html = fileInfoHtml(fi, mt, textSnippet);

    // Column view -> side pane
    if (mode == ViewMode::Column && previewLabel) {
        if (!pm.isNull()) {
            previewLabel->setPixmap(pm);
            previewLabel->setToolTip(path);
        } else {
            previewLabel->setPixmap(QPixmap());
            previewLabel->setText(html);
            previewLabel->setToolTip(path);
        }
        return;
    }

    // Icon/List views -> alert-style Get Info
    QMessageBox mb(this);
    mb.setWindowTitle(QString("Info — %1").arg(fi.fileName()));
    mb.setTextFormat(Qt::RichText);
    if (!pm.isNull()) {
        mb.setIconPixmap(pm);
        mb.setInformativeText(html);
    } else {
        mb.setText(html);
    }
    mb.setStandardButtons(QMessageBox::Ok);
    mb.exec();
}

inline void ColFM::openApp(const QString &path) {
    QProcess::startDetached(path);
}

inline void ColFM::openFile(const QModelIndex &idx) {
    if (!idx.isValid()) return;
    const QString path = model->filePath(idx);
    QFileInfo fi(path);

    if (fi.isDir()) {
        currentRoot = idx;
        if (crumbs) crumbs->setPath(path);
        setViewMode(mode);
        return;
    }

    if (fi.isExecutable()) {
        openApp(path);
        return;
    }

    // Open with system default application
    const bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    if (!ok) statusBar()->showMessage("Could not open", 2000);
}
