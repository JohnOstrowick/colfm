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

// ---- ColFM open/preview helpers (no duplicated rendering logic) ----

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

// Build a single HTML block used by both the RHS preview pane and the Get-Info dialog.
inline QString buildGetInfoHtml(const QFileInfo &fi, const QMimeType &mt, const QString &path) {
    // Optional image tag (scaled by attributes; loads from file URL)
    QString imgTag;
    if (mt.name().startsWith("image/") && fi.isFile()) {
        // Probe size to choose a sensible display width
        QImageReader r(path);
        r.setAutoTransform(true);
        QSize s = r.size();
        int w = s.width() > 0 ? s.width() : 512;
        int h = s.height() > 0 ? s.height() : 512;
        if (w > 512) { h = int(h * (512.0 / w)); w = 512; }
        imgTag = QString(
            "<div style='text-align:center; margin:4px 0 10px 0'>"
            "<img src=\"%1\" width=\"%2\" height=\"%3\" />"
            "</div>"
        ).arg(QUrl::fromLocalFile(path).toString()).arg(w).arg(h);
    }

    // Optional text snippet preview
    QString snippet;
    if (mt.name().startsWith("text/") && fi.isFile() && fi.size() <= 256*1024) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            QString sample = ts.read(4096);
            sample.replace('<', "&lt;").replace('>', "&gt;");
            snippet = "<pre style='white-space:pre-wrap; margin:6px 0 0 0'>" + sample + "</pre>";
        }
    } else if (mt.name().startsWith("audio/") || mt.name().startsWith("video/") ||
               mt.name().contains("officedocument") || mt.name().contains("msword") ||
               mt.name().contains("excel") || mt.name().contains("powerpoint")) {
        snippet = "(No inline preview. Use Open to launch in the default application.)";
    }

    const QString name  = fi.fileName().toHtmlEscaped();
    const QString type  = (mt.isValid() ? mt.name() : "unknown").toHtmlEscaped();
    const QString size  = fi.isDir() ? "-" : humanSize(fi.size());
    const QString mod   = fi.lastModified().toString(Qt::ISODate);
    const QString perms = permsToString(fi.permissions());
    const QString owner = fi.owner().toHtmlEscaped();
    const QString group = fi.group().toHtmlEscaped();
    const QString phtml = fi.absoluteFilePath().toHtmlEscaped();

    QString html;
    html += "<div style='font-family:Sans-Serif; font-size:12px; line-height:1.25'>";
    html += imgTag;
    html += "<table style='border-collapse:collapse' cellspacing='0' cellpadding='2'>";
    auto row = [&](const char *k, const QString &v){
        html += QString(
            "<tr>"
              "<td style='font-weight:bold; padding-right:10px; white-space:nowrap; vertical-align:top'>%1</td>"
              "<td style='width:100%%'>%2</td>"
            "</tr>"
        ).arg(k).arg(v);
    };
    row("Name", name);
    row("Kind", type);
    row("Size", size);
    row("Modified", mod);
    row("Permissions", perms);
    row("Owner", owner);
    row("Group", group);
    row("Path", "<tt>" + phtml + "</tt>");
    if (!snippet.isEmpty()) row("Preview", snippet);
    html += "</table></div>";
    return html;
}

// Single entry point used everywhere to show info.
// If inPane==true -> write to RHS preview pane. Else -> modal Get-Info dialog.
inline void showGetInfo(ColFM *self, const QString &path, bool inPane) {
    if (!self) return;
    QFileInfo fi(path);
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    const QString html = buildGetInfoHtml(fi, mt, path);

	if (inPane) { self->setPreviewHtml(html, path); return; }

    QMessageBox mb(self);
    mb.setWindowTitle(QString("Info — %1").arg(fi.fileName()));
    mb.setTextFormat(Qt::RichText);
    mb.setText(html);                                       // one rich-text payload (no duplication)
    mb.setStandardButtons(QMessageBox::Ok);
    mb.exec();
}

inline void ColFM::setPreviewHtml(const QString &html, const QString &path) {
    if (!previewLabel) return;
    previewLabel->setPixmap(QPixmap());
    previewLabel->setTextFormat(Qt::RichText);
    previewLabel->setText(html);
    previewLabel->setToolTip(path);
}

inline void ColFM::previewFile(const QModelIndex &idx) {
    if (!idx.isValid()) return;
    const QString path = model->filePath(idx);
    const bool usePane = (mode == ViewMode::Column);
    showGetInfo(this, path, usePane);
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
